//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REFERENCE_H
#define REFERENCE_H

#include "Repository.h"
#include "git2/revwalk.h"
#include <QSharedPointer>

struct git_reference;

namespace git {

class AnnotatedCommit;
class Commit;
class Object;
class RevWalk;

class Reference {
  Q_DECLARE_TR_FUNCTIONS(Reference)

public:
  Reference();

  bool isValid() const { return !d.isNull(); }
  explicit operator bool() const { return isValid(); }

  Repository repo() const;

  bool isTag() const;
  bool isBranch() const;
  bool isLocalBranch() const;
  bool isRemoteBranch() const;

  bool isDetachedHead() const;
  bool isHead() const;
  bool isStash() const;

  QString name(bool decorateDetachedHead = true) const;
  QString qualifiedName() const;

  // Create a walker over the referenced commit.
  RevWalk walker(int sort = GIT_SORT_NONE, bool firstCommitOnly = false) const;

  // Calculate difference in commits between this and the given reference.
  // References that have diverged calculate the distance to a common base.
  int difference(const Reference &ref) const;

  // Setting the commit doesn't mutate this reference. It returns a
  // new reference that points to the given commit. This reference
  // remains valid and continues to point to the same commit.
  Commit target() const;
  Reference setTarget(const Commit &commit, const QString &msg) const;

  // Get the annotated commit for merging this reference.
  AnnotatedCommit annotatedCommit() const;

  static bool isNameValid(const QString &name);

protected:
  Reference(git_reference *ref);
  operator git_reference *() const;

  QSharedPointer<git_reference> d;

  friend class Commit;
  friend class Repository;
  friend class RevWalk;
};

} // namespace git

Q_DECLARE_METATYPE(git::Reference);

#endif
