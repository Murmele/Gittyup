//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Reference.h"
#include "Commit.h"
#include "Object.h"
#include "RevWalk.h"
#include "Repository.h"
#include "TagRef.h"
#include "Tree.h"
#include "git2/annotated_commit.h"
#include "git2/branch.h"
#include "git2/checkout.h"
#include "git2/refs.h"
#include "git2/repository.h"
#include "git2/signature.h"

namespace git {

Reference::Reference() {}

Reference::Reference(git_reference *ref) : d(ref, git_reference_free) {}

Reference::operator git_reference *() const { return d.data(); }

Repository Reference::repo() const { return git_reference_owner(d.data()); }

bool Reference::isTag() const { return git_reference_is_tag(d.data()); }

bool Reference::isBranch() const {
  return (isLocalBranch() || isRemoteBranch());
}

bool Reference::isLocalBranch() const {
  return git_reference_is_branch(d.data());
}

bool Reference::isRemoteBranch() const {
  return git_reference_is_remote(d.data());
}

bool Reference::isDetachedHead() const { return (qualifiedName() == "HEAD"); }

bool Reference::isHead() const {
  return (isDetachedHead() || git_branch_is_head(d.data()));
}

bool Reference::isStash() const { return (qualifiedName() == "refs/stash"); }

QString Reference::name(bool decorateDetachedHead) const {
  if (isBranch()) {
    const char *name = nullptr;
    return !git_branch_name(&name, d.data()) ? name : QString();
  }

  Commit commit = target();
  if (isDetachedHead() && commit.isValid()) {
    QString name = commit.detachedHeadName();
    return !decorateDetachedHead ? name : tr("HEAD detached at %1").arg(name);
  }

  // Get shorthand.
  return git_reference_shorthand(d.data());
}

QString Reference::qualifiedName() const {
  return git_reference_name(d.data());
}

RevWalk Reference::walker(int sort, bool firstCommitOnly) const {
  Commit commit = target();
  return commit.isValid() ? commit.walker(sort, firstCommitOnly) : RevWalk();
}

int Reference::difference(const Reference &ref) const {
  Commit lhs = target();
  Commit rhs = ref.target();
  return (lhs && rhs) ? lhs.difference(rhs) : 0;
}

Commit Reference::target() const {
  git_object *obj = nullptr;
  git_reference_peel(&obj, d.data(), GIT_OBJECT_COMMIT);
  return Commit(reinterpret_cast<git_commit *>(obj));
}

Reference Reference::setTarget(const Commit &commit, const QString &msg) const {
  // Convert reflog message.
  QByteArray storage;
  const char *log = nullptr;
  if (!msg.isEmpty()) {
    storage = msg.toUtf8();
    log = storage.constData();
  }

  git_reference *ref = nullptr;
  git_reference_set_target(&ref, d.data(), commit, log);

  Reference result(ref);
  emit repo().notifier()->referenceUpdated(result);

  return result;
}

AnnotatedCommit Reference::annotatedCommit() const {
  git_annotated_commit *commit = nullptr;
  git_repository *repo = git_reference_owner(d.data());
  git_annotated_commit_from_ref(&commit, repo, d.data());
  return AnnotatedCommit(commit, repo);
}

bool Reference::isNameValid(const QString &name) {
  int valid;
  git_reference_name_is_valid(&valid, name.toUtf8());
  return valid;
}

} // namespace git
