//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RecentRepository.h"

RecentRepository::RecentRepository(const QString &path, QObject *parent)
    : QObject(parent), mPath(path) {}

QString RecentRepository::path() const { return mPath; }

QString RecentRepository::name() const {
  return mPath.section('/', -mSections);
}

void RecentRepository::increment() { ++mSections; }
