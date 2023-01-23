//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BLAME_H
#define BLAME_H

#include "git2/blame.h"
#include <QSharedPointer>

namespace git {

class Id;
class Signature;

class Blame {
public:
  class Callbacks {
  public:
    virtual ~Callbacks() {}

    virtual bool progress() { return false; }
  };

  Blame();

  bool isValid() const { return !d.isNull(); }

  int count() const;
  int index(int line) const;

  int line(int index) const;
  Id id(int index) const;
  QString message(int index) const;
  Signature signature(int index) const;

  bool isCommitted(int index) const;

  Blame updated(const QByteArray &buffer) const;

protected:
  Blame(git_blame *blame, git_repository *repo);

  git_repository *repo;
  QSharedPointer<git_blame> d;

  friend class Repository;
};

} // namespace git

#endif
