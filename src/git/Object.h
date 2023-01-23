//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef OBJECT_H
#define OBJECT_H

#include "git2/object.h"
#include <QSharedPointer>

namespace git {

class Id;
class Repository;

class Object {
public:
  bool isValid() const { return !d.isNull(); }
  explicit operator bool() const { return isValid(); }

  Repository repo() const;

  git_object_t type() const;

  Id id() const;
  QString shortId() const;
  QString link() const;

  bool operator==(const Object &rhs) const;
  bool operator!=(const Object &rhs) const;

protected:
  Object(git_object *obj = nullptr);
  operator const git_oid *() const;
  operator const git_object *() const;

  QSharedPointer<git_object> d;

  friend class Branch;
  friend class Reference;
  friend class Repository;
  friend class Tree;
};

uint qHash(const Object &key);

} // namespace git

#endif
