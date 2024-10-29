//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COMMIT_H
#define COMMIT_H

#include "Object.h"
#include "git2/commit.h"
#include "git2/revwalk.h"
#include "git2/reset.h"
#include "Blob.h"

class QDateTime;

namespace git {

class AnnotatedCommit;
class Diff;
class Reference;
class RevWalk;
class Signature;
class Tree;

class Commit : public Object {
public:
  enum MessageOption { NoMessageOption = 0x0, SubstituteEmoji = 0x1 };

  Q_DECLARE_FLAGS(MessageOptions, MessageOption);

  Commit();

  bool isMerge() const;

  QString summary(MessageOptions options = NoMessageOption) const;
  QString body(MessageOptions options = NoMessageOption) const;
  QString message(MessageOptions options = NoMessageOption) const;
  QString description() const;

  QString detachedHeadName() const;

  Signature author() const;
  Signature committer() const;

  Diff diff(const Commit &commit = git::Commit(), int contextLines = -1,
            bool ignoreWhitespace = false) const;
  Tree tree() const;
  QList<Commit> parents() const;

  // Get refs that point to this commit.
  QList<Reference> refs() const;

  // Create a walker starting from this commit.
  RevWalk walker(int sort = GIT_SORT_NONE, bool firstCommitOnly = false) const;

  // Calculate difference in commits between this and the given commit.
  // Commits that have diverged calculate the distance to a common base.
  int difference(const Commit &commit) const;

  // Revert this commit in the index and workdir.
  bool revert() const;

  bool amend(const Signature &author, const Signature &committer,
             const QString &commitMessage, const Tree &tree) const;

  // Reset HEAD to this commit.
  bool reset(git_reset_t type = GIT_RESET_MIXED,
             const QStringList &paths = QStringList(),
             bool triggerReferenceUpdated = true) const;

  // favorite commits
  bool isStarred() const;
  void setStarred(bool starred);

  // Get the annotated commit for merging this commit.
  AnnotatedCommit annotatedCommit() const;

  // Set path to emoji description file. This should be set before
  // any commits are created and is not expected to change.
  static void setEmojiFile(const QString &file);

  Blob blob(const QString &file) const;

private:
  Commit(git_commit *commit);
  operator git_commit *() const;

  QString decodeMessage(const char *msg) const;
  QString substituteEmoji(const QString &text) const;

  friend class Blame;
  friend class AnnotatedCommit;
  friend class Rebase;
  friend class Reference;
  friend class Repository;
  friend class RevWalk;
};

} // namespace git

Q_DECLARE_METATYPE(git::Commit);

#endif
