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
#include <QMap>
#include <QThread>
#include <poll.h>
#include <unistd.h>

namespace {

// FIXME: Include hidden and filter .git explicitly?
const QDir::Filters kFilters = (QDir::Dirs | QDir::NoDotAndDotDot);

} // namespace

class RepositoryWatcherPrivate : public QObject {
  Q_OBJECT

public:
  RepositoryWatcherPrivate(const git::Repository &repo,
                           QObject *parent = nullptr)
      : QObject(parent), mRepo(repo), mFSWatcher(parent) {
    connect(&mFSWatcher, &QFileSystemWatcher::directoryChanged, this,
            &RepositoryWatcherPrivate::directoryChanged);
    connect(&mFSWatcher, &QFileSystemWatcher::fileChanged, this,
            &RepositoryWatcherPrivate::fileChanged);
    watch(mRepo.workdir());
  }

  ~RepositoryWatcherPrivate() {}

  bool isValid() const { return true; }

  void directoryChanged(const QString &path) {
    if (!mRepo.isIgnored(path)) {
      // Start watching new directories.
      if (QDir(path).exists())
        watch(path);
      emit notificationReceived();
    }
  }

  void fileChanged(const QString &path) { emit notificationReceived(); }

  void watch(const QDir &dir) {
    mFSWatcher.addPath(dir.path().toUtf8());

    // Watch subdirs.
    foreach (const QString &name, dir.entryList(kFilters)) {
      QString path = dir.filePath(name);
      if (!mRepo.isIgnored(path))
        watch(path);
    }
  }

signals:
  void notificationReceived();

private:
  git::Repository mRepo;
  QFileSystemWatcher mFSWatcher;
};

RepositoryWatcher::RepositoryWatcher(const git::Repository &repo,
                                     QObject *parent)
    : QObject(parent), d(new RepositoryWatcherPrivate(repo, this)) {
  init(repo);
  connect(d, &RepositoryWatcherPrivate::notificationReceived, &mTimer,
          static_cast<void (QTimer::*)()>(&QTimer::start));
}

RepositoryWatcher::~RepositoryWatcher() {}

#include "RepositoryWatcher_qt.moc"
