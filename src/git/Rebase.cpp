//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Rebase.h"
#include "Commit.h"
#include "Reference.h"
#include "Repository.h"
#include "Signature.h"
#include "git2/checkout.h"
#include "git2/errors.h"
#include "git2/merge.h"
#include "git2/rebase.h"

namespace git {

Rebase::Rebase(
  git_repository *repo,
  git_rebase *rebase,
  const QString &overrideUser,
  const QString &overrideEmail
)
  : mRepo(repo),
  d(rebase, git_rebase_free),
  mOverrideUser(overrideUser),
  mOverrideEmail(overrideEmail)
{}

int Rebase::count() const
{
  return git_rebase_operation_entrycount(d.data());
}

bool Rebase::hasNext() const
{
  int index = git_rebase_operation_current(d.data());
  int count = git_rebase_operation_entrycount(d.data());
  return (count > 0 && (index == GIT_REBASE_NO_OPERATION || index < count - 1));
}

Commit Rebase::next()
{
  git_rebase_operation *op = nullptr;
  if (git_rebase_next(&op, d.data()))
    return Commit();

  git_commit *commit = nullptr;
  git_commit_lookup(&commit, mRepo, &op->id);
  return Commit(commit);
}

Commit Rebase::commit()
{
  git_oid id;
  git_rebase *ptr = d.data();

  Signature sig = Repository(mRepo).defaultSignature(
    nullptr,
    mOverrideUser,
    mOverrideEmail
  );

  if (int err = git_rebase_commit(&id, ptr, nullptr, sig, nullptr, nullptr)) {
    if (err != GIT_EAPPLIED)
      return Commit();

    // Look up the existing commit.
    int index = git_rebase_operation_current(ptr);
    git_rebase_operation *op = git_rebase_operation_byindex(ptr, index);

    git_commit *commit = nullptr;
    git_commit_lookup(&commit, mRepo, &op->id);
    return Commit(commit);
  }

  git_commit *commit = nullptr;
  git_commit_lookup(&commit, mRepo, &id);
  return Commit(commit);
}

void Rebase::abort()
{
  Repository repo(mRepo);
  int state = repo.state();
  git_rebase_abort(d.data());
  if (repo.state() != state)
    emit repo.notifier()->stateChanged();
}

bool Rebase::finish()
{
  Repository repo(mRepo);

  int error = git_rebase_finish(d.data(), repo.defaultSignature(
    nullptr,
    mOverrideUser,
    mOverrideEmail
  ));

  emit repo.notifier()->referenceUpdated(repo.head());
  return !error;
}

} // namespace git
