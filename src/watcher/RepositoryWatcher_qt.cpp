//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: François Revol
//

#include "RepositoryWatcher.h"
#include <QFileSystemWatcher>

namespace {

// `.git` is excluded by isIgnored().
const QDir::Filters kFilters =
    (QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);

} // namespace

// Only directories are watched, so edits to existing files go unnoticed.
class QtRepositoryWatcher : public RepositoryWatcher {
public:
  QtRepositoryWatcher(const git::Repository &repo, QObject *parent)
      : RepositoryWatcher(repo, parent), mRepo(repo) {
    connect(&mFSWatcher, &QFileSystemWatcher::directoryChanged, this,
            &QtRepositoryWatcher::directoryChanged);
    watch(mRepo.workdir());
  }

private:
  void directoryChanged(const QString &path) {
    if (mRepo.isIgnored(path))
      return;

    // Start watching new directories.
    if (QDir(path).exists())
      watch(path);

    scheduleNotification();
  }

  void watch(const QDir &dir) {
    mFSWatcher.addPath(dir.path());

    // Watch subdirs.
    for (const QString &name : dir.entryList(kFilters)) {
      QString path = dir.filePath(name);
      if (!mRepo.isIgnored(path))
        watch(path);
    }
  }

  git::Repository mRepo;
  QFileSystemWatcher mFSWatcher;
};

RepositoryWatcher *RepositoryWatcher::create(const git::Repository &repo,
                                             QObject *parent) {
  return new QtRepositoryWatcher(repo, parent);
}
