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
    : QObject(parent), mPath(gitpath) {}

QString RecentRepository::gitpath() const { return mPath; }

QString RecentRepository::name() const {
  if (mPath.endsWith("/.git"))
    return mPath.section('/', -mSections - 1, -2);
  else
    return mPath.section('/', -mSections, -1);
}

void RecentRepository::increment() { ++mSections; }
