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
#include <QMap>
#include <QThread>
#include <poll.h>
#include <unistd.h>
#include <sys/inotify.h>

namespace {

const uint kFlags =
    (IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_DELETE_SELF |
     IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF);

// `.git` is excluded by isIgnored().
const QDir::Filters kFilters =
    (QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);

} // namespace

class InotifyThread : public QThread {
  Q_OBJECT

public:
  explicit InotifyThread(const git::Repository &repo)
      : mRepo(repo), mFilter(repo) {
    mFd = inotify_init1(IN_NONBLOCK);
    if (mFd < 0)
      return; // FIXME: Report error?

    if (pipe(mPipe) < 0) {
      // FIXME: Report error?
      close(mFd);
      mFd = -1;
    }
  }

  ~InotifyThread() {
    close(mPipe[1]);
    close(mPipe[0]);
    close(mFd);
  }

  bool isValid() const { return (mFd >= 0); }

  void run() override {
    // Watch the root directory.
    watch(mRepo.workdir());

    // Start listening for notifications.
    forever {
      pollfd pollFds[2];
      pollFds[0].fd = mPipe[0];
      pollFds[0].events = POLLIN;
      pollFds[1].fd = mFd;
      pollFds[1].events = POLLIN;
      if (poll(pollFds, 2, -1) < 0)
        return; // FIXME: Report error?

      // Check for signal to quit.
      if (pollFds[0].revents & POLLIN)
        return;

      // Check for notifications.
      if (!(pollFds[1].revents & POLLIN))
        continue;

      // Read notifications.
      bool ignored = true;
      forever {
        char buf[4096];
        int len = read(mFd, buf, sizeof(buf));
        if (len <= 0)
          break;

        const inotify_event *event = nullptr;
        for (char *ptr = buf; ptr < buf + len;
             ptr += sizeof(inotify_event) + event->len) {
          event = reinterpret_cast<inotify_event *>(ptr);
          if (event->len) {
            QString path = mWds.value(event->wd).filePath(event->name);
            if (mFilter.isRelevant(path)) {
              ignored = false;

              // Start watching new directories.
              uint32_t mask = (IN_CREATE | IN_ISDIR);
              if ((event->mask & mask) == mask)
                watch(path);
            }
          }
        }
      }

      if (!ignored)
        emit notificationReceived();
    }
  }

  void watch(const QDir &dir) {
    int wd = inotify_add_watch(mFd, dir.path().toUtf8(), kFlags);
    if (wd < 0)
      return; // FIXME: Report error?

    // Associate the dir with this watch descriptor.
    mWds[wd] = dir;

    // Watch subdirs.
    for (const QString &name : dir.entryList(kFilters)) {
      QString path = dir.filePath(name);
      if (mFilter.isRelevant(path))
        watch(path);
    }
  }

  void stop() {
    if (write(mPipe[1], "\n", 1) < 0)
      terminate(); // FIXME: Report error?
  }

signals:
  void notificationReceived();

private:
  git::Repository mRepo;
  PathFilter mFilter;
  int mFd = -1;
  int mPipe[2] = {-1, -1};
  QMap<int, QDir> mWds;
};

class LinuxRepositoryWatcher : public RepositoryWatcher {

public:
  LinuxRepositoryWatcher(const git::Repository &repo, QObject *parent)
      : RepositoryWatcher(repo, parent), mThread(repo) {
    connect(&mThread, &InotifyThread::notificationReceived, this,
            &LinuxRepositoryWatcher::scheduleNotification);
    if (mThread.isValid())
      mThread.start();
  }

  ~LinuxRepositoryWatcher() override {
    if (mThread.isValid()) {
      mThread.stop();
      mThread.wait();
    }
  }

private:
  InotifyThread mThread;
};

RepositoryWatcher *RepositoryWatcher::create(const git::Repository &repo,
                                             QObject *parent) {
  return new LinuxRepositoryWatcher(repo, parent);
}

#include "RepositoryWatcher_linux.moc"
