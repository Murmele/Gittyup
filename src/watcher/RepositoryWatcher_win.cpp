//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "PathFilter.h"
#include "RepositoryWatcher.h"
#include <QThread>
#include <QVector>
#include <windows.h>

namespace {

const uint kFlags =
    (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
     FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
     FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
     FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY);

} // namespace

class DirectoryChangesThread : public QThread {
  Q_OBJECT

public:
  explicit DirectoryChangesThread(const git::Repository &repo)
      : mFilter(repo), mBuffer(16 * 1024) {
    // Pass this to callback.
    ZeroMemory(&mOverlapped, sizeof(OVERLAPPED));
    mOverlapped.hEvent = this;

    // Create event to signal the thread to quit.
    mStop = CreateEvent(0, false, false, 0);

    // Open directory.
    QString path = QDir::toNativeSeparators(repo.workdir().path());
    mHandle =
        CreateFileW((wchar_t *)path.utf16(), FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
  }

  ~DirectoryChangesThread() {
    CloseHandle(mHandle);
    CloseHandle(mStop);
  }

  PathFilter &filter() { return mFilter; }
  QVector<BYTE> buffer() const { return mBuffer; }

  void run() override {
    // Start watching for notifications.
    watch();

    // Wait on the stop event.
    forever {
      switch (WaitForSingleObjectEx(mStop, INFINITE, true)) {
        case WAIT_OBJECT_0:
          return;

        case WAIT_IO_COMPLETION:
          break;

        default:
          // no-op
          break; // FIXME: Report error?
      }
    }
  }

  void watch() {
    ReadDirectoryChangesW(mHandle, mBuffer.data(), mBuffer.size(), true, kFlags,
                          nullptr, &mOverlapped, &notify);
  }

  void stop() {
    // Signal the thread to quit.
    SetEvent(mStop);
  }

  static void CALLBACK notify(DWORD errorCode, DWORD numBytes,
                              LPOVERLAPPED overlapped) {
    if (errorCode || !numBytes)
      return; // FIXME: Report error?

    // Copy buffer and restart.
    DirectoryChangesThread *watcher =
        static_cast<DirectoryChangesThread *>(overlapped->hEvent);
    QVector<BYTE> buffer = watcher->buffer();
    watcher->watch();

    // Iterate over notifications.
    const BYTE *ptr = buffer.constData();
    forever {
      const FILE_NOTIFY_INFORMATION *info =
          reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(ptr);

      int size = info->FileNameLength / sizeof(wchar_t);
      QString native = QString::fromWCharArray(info->FileName, size);
      QString path = QDir::fromNativeSeparators(native);
      if (!path.isEmpty() && watcher->filter().isRelevant(path)) {
        emit watcher->notificationReceived();
        return;
      }

      if (!info->NextEntryOffset)
        return;

      ptr += info->NextEntryOffset;
    }
  }

signals:
  void notificationReceived();

private:
  PathFilter mFilter;
  HANDLE mStop;
  HANDLE mHandle;
  QVector<BYTE> mBuffer;
  OVERLAPPED mOverlapped;
};

class WindowsRepositoryWatcher : public RepositoryWatcher {
public:
  WindowsRepositoryWatcher(const git::Repository &repo, QObject *parent)
      : RepositoryWatcher(repo, parent), mThread(repo) {
    connect(&mThread, &DirectoryChangesThread::notificationReceived, this,
            &WindowsRepositoryWatcher::scheduleNotification);
    mThread.start();
  }

  ~WindowsRepositoryWatcher() override {
    mThread.stop();
    mThread.wait();
  }

private:
  DirectoryChangesThread mThread;
};

RepositoryWatcher *RepositoryWatcher::create(const git::Repository &repo,
                                             QObject *parent) {
  return new WindowsRepositoryWatcher(repo, parent);
}

#include "RepositoryWatcher_win.moc"
