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

class RepositoryWatcher : public QObject {
public:
  static RepositoryWatcher *create(const git::Repository &repo,
                                   QObject *parent = nullptr);

  ~RepositoryWatcher() override = default;
  void setDebounceInterval(int msec);
  void cancelPendingNotification();

protected:
  RepositoryWatcher(const git::Repository &repo, QObject *parent);

  void scheduleNotification();

private:
  QTimer mTimer;
};

#endif
