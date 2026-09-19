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

class MacRepositoryWatcher : public RepositoryWatcher {
public:
  MacRepositoryWatcher(const git::Repository &repo, QObject *parent)
      : RepositoryWatcher(repo, parent), mRepo(repo) {
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

  ~MacRepositoryWatcher() override {
    // Stop stream.
    FSEventStreamStop(mStream);

    // Release stream.
    FSEventStreamInvalidate(mStream);
    FSEventStreamRelease(mStream);

    // Release queue
    dispatch_release(mQueue);
  }

private:
  static void notify(ConstFSEventStreamRef streamRef, void *clientCallBackInfo,
                     size_t numEvents, void *eventPaths,
                     const FSEventStreamEventFlags eventFlags[],
                     const FSEventStreamEventId eventIds[]) {
    MacRepositoryWatcher *watcher =
        static_cast<MacRepositoryWatcher *>(clientCallBackInfo);

    // Filter out ignored directories.
    const char **paths = static_cast<const char **>(eventPaths);
    for (size_t i = 0; i < numEvents; ++i) {
      if (!watcher->mRepo.isIgnored(paths[i])) {
        // This runs on the dispatch queue; the timer lives on the main thread.
        QMetaObject::invokeMethod(
            watcher, [watcher] { watcher->scheduleNotification(); },
            Qt::QueuedConnection);
        return;
      }
    }
  }

  git::Repository mRepo;
  dispatch_queue_t mQueue;
  FSEventStreamRef mStream;
};

RepositoryWatcher *RepositoryWatcher::create(const git::Repository &repo,
                                             QObject *parent) {
  return new MacRepositoryWatcher(repo, parent);
}
