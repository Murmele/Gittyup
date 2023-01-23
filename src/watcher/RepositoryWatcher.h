//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REPOSITORYWATCHER_H
#define REPOSITORYWATCHER_H

#include "git/Repository.h"
#include <QObject>
#include <QTimer>

class RepositoryWatcherPrivate;

class RepositoryWatcher : public QObject {
public:
  RepositoryWatcher(const git::Repository &repo, QObject *parent = nullptr);
  ~RepositoryWatcher() override;

  void init(const git::Repository &repo);
  void cancelPendingNotification();

private:
  QTimer mTimer;
  RepositoryWatcherPrivate *d;
};

#endif
