//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REBASE_H
#define REBASE_H

#include <QSharedPointer>

// TODO: move to cpp again, forward declaration should be enough
#include "git2/rebase.h"

struct git_rebase;
struct git_repository;

namespace git {

class Commit;

class Rebase {
public:
  bool isValid() const { return !d.isNull(); }

  int count() const;
  size_t currentIndex() const;
  const git_rebase_operation *operation(size_t index);
  Commit commitToRebase() const;
  bool hasNext() const;
  Commit next() const;
  Commit commit(const QString &message);

  void abort();
  bool finish();

private:
  Rebase();
  Rebase(git_repository *repo, git_rebase *rebase = nullptr,
         const QString &overrideUser = QString(),
         const QString &overrideEmail = QString());

  git_repository *mRepo;
  QSharedPointer<git_rebase> d;
  QString mOverrideUser;
  QString mOverrideEmail;

  friend class Repository;
};

} // namespace git

#endif
