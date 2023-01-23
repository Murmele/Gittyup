//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Tree.h"
#include "Index.h"
#include "git2/pathspec.h"

namespace git {

Tree::Tree() : Object() {}

Tree::Tree(const Object &rhs) : Object(rhs) {
  if (isValid() && type() != GIT_OBJECT_TREE)
    d.clear();
}

Tree::Tree(git_tree *tree) : Object(reinterpret_cast<git_object *>(tree)) {}

Tree::operator git_tree *() const {
  return reinterpret_cast<git_tree *>(d.data());
}

int Tree::count() const { return git_tree_entrycount(*this); }

QString Tree::name(int index) const {
  const git_tree_entry *entry = git_tree_entry_byindex(*this, index);
  return git_tree_entry_name(entry);
}

Object Tree::object(int index) const {
  git_object *obj = nullptr;
  const git_tree_entry *entry = git_tree_entry_byindex(*this, index);
  git_tree_entry_to_object(&obj, git_object_owner(d.data()), entry);
  return Object(obj);
}

Id Tree::id(const QString &path) const {
  git_tree_entry *entry = nullptr;
  if (git_tree_entry_bypath(&entry, *this, path.toUtf8()))
    return Id();

  Id id(git_tree_entry_id(entry));
  git_tree_entry_free(entry);
  return id;
}

} // namespace git
