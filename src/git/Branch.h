//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BRANCH_H
#define BRANCH_H

#include "Reference.h"

namespace git {

class Commit;
class AnnotatedCommit;
class Remote;

class Branch : public Reference {
public:
  Branch();
  Branch(const Reference &rhs);

  // Valid for local branches.
  Branch upstream() const;
  void setUpstream(const Branch &upstream);

  // Is this local branch up-to-date with its upstream?
  // Branches that don't have an upstream are compared with HEAD.
  bool isMerged() const;

  // Valid for both local and remote tracking branches. Local branches use
  // the upstream remote tracking branch, if it exists, to get the remote.
  Remote remote() const;

  // Valid for local branches.
  Branch rename(const QString &name);

  // Valid for local and remote branches.
  void remove(bool force = false);

  bool isRebase() const;
  void setRebase(bool checked);

  // Create a merge head for this local branch.
  AnnotatedCommit annotatedCommitFromFetchHead() const;

  static bool isNameValid(const QString &name);

private:
  Branch(git_reference *ref);

  friend class Repository;
};

} // namespace git

Q_DECLARE_METATYPE(git::Branch);

#endif
