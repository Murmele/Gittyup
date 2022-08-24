//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AnnotatedCommit.h"
#include "Branch.h"
#include "Commit.h"
#include "Config.h"
#include "Diff.h"
#include "Remote.h"
#include "Repository.h"
#include "RevWalk.h"
#include "git2/buffer.h"
#include "git2/branch.h"
#include "git2/merge.h"
#include "git2/refs.h"
#include "git2/remote.h"
#include "git2/signature.h"

namespace git {

namespace {

const QString kRebaseFmt = "branch.%1.rebase";

} // namespace

Branch::Branch() : Reference() {}

Branch::Branch(git_reference *ref) : Reference(ref) {
  if (isValid() && !isBranch())
    d.clear();
}

Branch::Branch(const Reference &rhs) : Reference(rhs) {
  if (isValid() && !isBranch())
    d.clear();
}

Branch Branch::upstream() const {
  Q_ASSERT(isLocalBranch());

  git_reference *ref = nullptr;
  git_branch_upstream(&ref, d.data());
  return Branch(ref);
}

void Branch::setUpstream(const Branch &upstream) {
  Q_ASSERT(isLocalBranch());

  QByteArray buffer;
  if (upstream.isValid())
    buffer = upstream.name().toUtf8();

  const char *name = !buffer.isEmpty() ? buffer.constData() : nullptr;
  git_branch_set_upstream(d.data(), name);

  emit repo().notifier()->referenceUpdated(*this);
}

bool Branch::isMerged() const {
  if (!isLocalBranch())
    return false;

  Branch branch = upstream();
  if (!branch.isValid())
    branch = repo().head();

  if (!branch.isValid())
    return false;

  return !difference(branch);
}

Remote Branch::remote() const {
  if (isLocalBranch()) {
    Branch up = upstream();
    return up.isValid() ? up.remote() : Remote();
  }

  git_buf buf = GIT_BUF_INIT;
  git_repository *repo = git_reference_owner(d.data());
  if (git_branch_remote_name(&buf, repo, qualifiedName().toUtf8()))
    return Remote();

  git_remote *remote = nullptr;
  if (git_remote_lookup(&remote, repo, buf.ptr)) {
    git_buf_dispose(&buf);
    return Remote();
  }

  git_buf_dispose(&buf);
  return Remote(remote);
}

Branch Branch::rename(const QString &name) {
  Q_ASSERT(isLocalBranch());

  git_reference *ref = nullptr;
  if (git_branch_move(&ref, d.data(), name.toUtf8(), false))
    return Branch();

  // Invalidate this branch.
  d.clear();

  Branch branch(ref);
  emit branch.repo().notifier()->referenceUpdated(branch);
  return branch;
}

void Branch::remove(bool force) {
  // Remember name.
  QString name = this->name();

  Repository repo = this->repo();
  emit repo.notifier()->referenceAboutToBeRemoved(*this);

  if (force ? git_reference_delete(d.data()) : git_branch_delete(d.data()))
    d.clear(); // Invalidate this branch.

  // We have to notify even if removal failed and the branch is still valid.
  // Clients can check this branch to see if the branch was really removed.
  emit repo.notifier()->referenceRemoved(name);
}

bool Branch::isRebase() const {
  return repo().config().value<bool>(kRebaseFmt.arg(name()));
}

void Branch::setRebase(bool checked) {
  repo().config().setValue(kRebaseFmt.arg(name()), checked);
}

AnnotatedCommit Branch::annotatedCommitFromFetchHead() const {
  Branch up = upstream();
  if (!up.isValid())
    return AnnotatedCommit();

  Remote remote = up.remote();
  if (!remote.isValid())
    return AnnotatedCommit();

  Commit commit = up.target();
  if (!commit.isValid())
    return AnnotatedCommit();

  git_annotated_commit *head = nullptr;
  QByteArray url = remote.url().toUtf8();
  QByteArray name = qualifiedName().toUtf8();
  git_repository *repo = git_reference_owner(d.data());
  if (git_annotated_commit_from_fetchhead(&head, repo, name, url, commit))
    return AnnotatedCommit();

  return AnnotatedCommit(head, repo);
}

bool Branch::isNameValid(const QString &name) {
  return Reference::isNameValid(QString("refs/heads/%1").arg(name));
}

} // namespace git
