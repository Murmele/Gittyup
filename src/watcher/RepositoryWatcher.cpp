//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RepositoryWatcher.h"

constexpr int kDefaultDebounceMsec = 2000;

RepositoryWatcher::RepositoryWatcher(const git::Repository &repo,
                                     QObject *parent)
    : QObject(parent) {
  // The timer has to run on the main thread.
  mTimer.setInterval(kDefaultDebounceMsec);
  mTimer.setSingleShot(true);
  connect(&mTimer, &QTimer::timeout, repo.notifier(),
          &git::RepositoryNotifier::workdirChanged);
}

void RepositoryWatcher::setDebounceInterval(int msec) {
  mTimer.setInterval(msec);
}

void RepositoryWatcher::cancelPendingNotification() { mTimer.stop(); }

void RepositoryWatcher::scheduleNotification() { mTimer.start(); }