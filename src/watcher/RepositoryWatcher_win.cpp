//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

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

// Never block the GUI thread indefinitely while shutting the watcher down.
const unsigned long kShutdownTimeoutMs = 2000;

} // namespace

class RepositoryWatcherPrivate : public QThread {
  Q_OBJECT

public:
  RepositoryWatcherPrivate(const git::Repository &repo,
                           QObject *parent = nullptr)
      : QThread(parent), mRepo(repo), mBuffer(16 * 1024) {
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

  ~RepositoryWatcherPrivate() {
    if (mHandle != INVALID_HANDLE_VALUE)
      CloseHandle(mHandle);
    if (mStop)
      CloseHandle(mStop);
  }

  git::Repository repo() const { return mRepo; }
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
    if (mHandle == INVALID_HANDLE_VALUE || mStopping.loadAcquire())
      return;

    ReadDirectoryChangesW(mHandle, mBuffer.data(), mBuffer.size(), true, kFlags,
                          nullptr, &mOverlapped, &notify);
  }

  void stop() {
    // Don't queue any further reads and drop the pending one, so no completion
    // routine can run while the thread is on its way out.
    mStopping.storeRelease(1);
    if (mHandle != INVALID_HANDLE_VALUE)
      CancelIoEx(mHandle, &mOverlapped);

    // Signal the thread to quit.
    SetEvent(mStop);
  }

  bool isStopping() const { return mStopping.loadAcquire(); }

  static void CALLBACK notify(DWORD errorCode, DWORD numBytes,
                              LPOVERLAPPED overlapped) {
    if (errorCode || !numBytes)
      return; // FIXME: Report error?

    // Copy buffer and restart.
    RepositoryWatcherPrivate *watcher =
        static_cast<RepositoryWatcherPrivate *>(overlapped->hEvent);
    if (watcher->isStopping())
      return;

    QVector<BYTE> buffer = watcher->buffer();
    watcher->watch();

    // Iterate over notifications.
    git::Repository repo = watcher->repo();
    const BYTE *ptr = buffer.constData();
    forever {
      const FILE_NOTIFY_INFORMATION *info =
          reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(ptr);

      int size = info->FileNameLength / sizeof(wchar_t);
      QString native = QString::fromWCharArray(info->FileName, size);
      QString path = QDir::fromNativeSeparators(native);
      if (!path.isEmpty() && !repo.isIgnored(path)) {
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
  git::Repository mRepo;
  HANDLE mStop;
  HANDLE mHandle;
  QVector<BYTE> mBuffer;
  OVERLAPPED mOverlapped;
  QAtomicInt mStopping{0};
};

RepositoryWatcher::RepositoryWatcher(const git::Repository &repo,
                                     QObject *parent)
    : QObject(parent), d(new RepositoryWatcherPrivate(repo, this)) {
  init(repo);
  connect(d, &RepositoryWatcherPrivate::notificationReceived, &mTimer,
          static_cast<void (QTimer::*)()>(&QTimer::start));

  d->start();
}

RepositoryWatcher::~RepositoryWatcher() {
  d->stop();

  // Bounded wait: a completion routine that never returns must not be able to
  // hang the whole application while the user is closing a window.
  if (!d->wait(kShutdownTimeoutMs)) {
    d->terminate();
    d->wait(kShutdownTimeoutMs);
  }
}

#include "RepositoryWatcher_win.moc"
