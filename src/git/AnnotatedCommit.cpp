//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AnnotatedCommit.h"
#include "Commit.h"
#include "Repository.h"
#include "git2/annotated_commit.h"
#include "git2/merge.h"

namespace git {

AnnotatedCommit::AnnotatedCommit() {}

AnnotatedCommit::AnnotatedCommit(git_annotated_commit *commit,
                                 git_repository *repo)
    : repo(repo), d(commit, git_annotated_commit_free) {}

AnnotatedCommit::operator git_annotated_commit *() const { return d.data(); }

Commit AnnotatedCommit::commit() const {
  git_commit *commit = nullptr;
  git_commit_lookup(&commit, repo, git_annotated_commit_id(d.data()));
  return Commit(commit);
}

int AnnotatedCommit::analysis() const {
  const git_annotated_commit *head = d.data();
  git_merge_analysis_t analysis = GIT_MERGE_ANALYSIS_NONE;
  git_merge_preference_t preference = GIT_MERGE_PREFERENCE_NONE;
  git_merge_analysis(&analysis, &preference, repo, &head, 1);
  return analysis;
}

} // namespace git
