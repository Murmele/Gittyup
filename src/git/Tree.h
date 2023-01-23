//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TREE_H
#define TREE_H

#include "Object.h"
#include "git2/tree.h"

namespace git {

class Tree : public Object {
public:
  Tree();
  Tree(const Object &rhs);

  int count() const;
  QString name(int index) const;
  Object object(int index) const;

  Id id(const QString &path) const;

private:
  Tree(git_tree *commit);
  operator git_tree *() const;

  friend class Commit;
  friend class Index;
  friend class Reference;
  friend class Repository;
};

} // namespace git

#endif
