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

struct git_rebase;
struct git_repository;

namespace git {

class Commit;

class Rebase
{
public:
  bool isValid() const { return !d.isNull(); }

  int count() const;
  bool hasNext() const;
  Commit next();
  Commit commit();

  void abort();
  bool finish();

private:
  Rebase(
    git_repository *repo,
    git_rebase *rebase = nullptr,
    const QString &overrideUser = QString(),
    const QString &overrideEmail = QString()
  );

  git_repository *mRepo;
  QSharedPointer<git_rebase> d;
  QString mOverrideUser;
  QString mOverrideEmail;

  friend class Repository;
};

} // namespace git

#endif
