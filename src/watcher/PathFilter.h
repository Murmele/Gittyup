//
//          Copyright (c) 2026, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Alf Henrik Sauge
//

#ifndef PATHFILTER_H
#define PATHFILTER_H

#include "git/Index.h"
#include "git/Repository.h"
#include <QDir>

// Decides which changed paths deserve a refresh. It owns its repository handle
// so it can be used from a watcher thread, but by one thread at a time.
class PathFilter {
public:
  explicit PathFilter(const git::Repository &repo);

  // Accepts absolute or workdir-relative paths.
  bool isRelevant(const QString &path);

private:
  QDir mWorkdir;
  git::Repository mRepo;
  git::Index mIndex;
};

#endif
