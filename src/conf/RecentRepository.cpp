//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RecentRepository.h"

RecentRepository::RecentRepository(const QString &gitpath, QObject *parent)
    : QObject(parent), mGitPath(gitpath) {}

QString RecentRepository::gitpath() const { return mGitPath; }

QString RecentRepository::name() const {
  QString name;
  if (mGitPath.endsWith("/.git"))
    name = mGitPath.section('/', -mSections - 1, -2);
  else
    name = mGitPath.section('/', -mSections, -1);
  return name;
}

void RecentRepository::increment() { ++mSections; }
