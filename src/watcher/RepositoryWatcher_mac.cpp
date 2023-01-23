//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RepositoryWatcher.h"
#include <CoreServices/CoreServices.h>

class RepositoryWatcherPrivate : public QObject {
  Q_OBJECT

public:
  RepositoryWatcherPrivate(const git::Repository &repo,
                           QObject *parent = nullptr)
      : QObject(parent), mRepo(repo) {
    // Create dispatch queue.
    mQueue = dispatch_queue_create("com.gittyup.RepositoryWatcher", nullptr);

    // Create stream to watch the workdir.
    FSEventStreamContext context = {0, this, nullptr, nullptr, nullptr};

    CFStringRef wd = repo.workdir().path().toCFString();
    CFArrayRef wds = CFArrayCreate(nullptr, (const void **)&wd, 1, nullptr);
    mStream = FSEventStreamCreate(nullptr, &notify, &context, wds,
                                  kFSEventStreamEventIdSinceNow, 0,
                                  kFSEventStreamCreateFlagNone);
    CFRelease(wds);
    CFRelease(wd);

    // Exclude the .git dir.
    CFStringRef gd = repo.dir().path().toCFString();
    CFArrayRef gds = CFArrayCreate(nullptr, (const void **)&gd, 1, nullptr);
    FSEventStreamSetExclusionPaths(mStream, gds);
    CFRelease(gds);
    CFRelease(gd);

    // Register with queue.
    FSEventStreamSetDispatchQueue(mStream, mQueue);

    // Start the stream.
    FSEventStreamStart(mStream);
  }

  ~RepositoryWatcherPrivate() {
    // Stop stream.
    FSEventStreamStop(mStream);

    // Release stream.
    FSEventStreamInvalidate(mStream);
    FSEventStreamRelease(mStream);

    // Release queue
    dispatch_release(mQueue);
  }

  git::Repository repo() const { return mRepo; }

  static void notify(ConstFSEventStreamRef streamRef, void *clientCallBackInfo,
                     size_t numEvents, void *eventPaths,
                     const FSEventStreamEventFlags eventFlags[],
                     const FSEventStreamEventId eventIds[]) {
    RepositoryWatcherPrivate *watcher =
        static_cast<RepositoryWatcherPrivate *>(clientCallBackInfo);

    // Filter out ignored directories.
    git::Repository repo = watcher->repo();
    const char **paths = static_cast<const char **>(eventPaths);
    for (int i = 0; i < numEvents; ++i) {
      if (!repo.isIgnored(paths[i])) {
        emit watcher->notificationReceived();
        return;
      }
    }
  }

signals:
  void notificationReceived();

private:
  git::Repository mRepo;
  dispatch_queue_t mQueue;
  FSEventStreamRef mStream;
};

RepositoryWatcher::RepositoryWatcher(const git::Repository &repo,
                                     QObject *parent)
    : QObject(parent), d(new RepositoryWatcherPrivate(repo, this)) {
  init(repo);
  connect(d, &RepositoryWatcherPrivate::notificationReceived, &mTimer,
          QOverload<>::of(&QTimer::start));
}

RepositoryWatcher::~RepositoryWatcher() {}

#include "RepositoryWatcher_mac.moc"
