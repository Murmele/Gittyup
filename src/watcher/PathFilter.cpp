//
//          Copyright (c) 2026, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#include "PathFilter.h"

PathFilter::PathFilter(const git::Repository &repo)
    : mWorkdir(repo.workdir()), mRepo(git::Repository::open(mWorkdir.path())) {
  if (mRepo.isValid())
    mIndex = mRepo.index();
}

bool PathFilter::isRelevant(const QString &path) {
  // Without a handle of our own, over-report rather than drop changes.
  if (!mRepo.isValid())
    return true;

  // libgit2 expects workdir-relative paths.
  QString relative =
      QDir::isAbsolutePath(path) ? mWorkdir.relativeFilePath(path) : path;
  if (!mRepo.isIgnored(relative))
    return true;

  // Ignore rules don't apply to tracked files.
  if (!mIndex.isValid())
    return true;

  mIndex.read();
  return mIndex.isTracked(relative);
}
