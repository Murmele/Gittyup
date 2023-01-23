//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Blame.h"
#include "Commit.h"
#include "Id.h"
#include "Signature.h"

namespace git {

Blame::Blame() {}

Blame::Blame(git_blame *blame, git_repository *repo)
    : repo(repo), d(blame, git_blame_free) {}

int Blame::count() const { return git_blame_get_hunk_count(d.data()); }

int Blame::index(int line) const {
  // binary search
  int min = 0;
  int max = count() - 1;
  while (min < max) {
    int mid = min + ((max - min) / 2);
    if (line < this->line(mid)) {
      max = mid - 1;
    } else if (line >= this->line(mid + 1)) {
      min = mid + 1;
    } else {
      return mid;
    }
  }

  return min;
}

int Blame::line(int index) const {
  return git_blame_get_hunk_byindex(d.data(), index)->final_start_line_number;
}

Id Blame::id(int index) const {
  return git_blame_get_hunk_byindex(d.data(), index)->final_commit_id;
}

QString Blame::message(int index) const {
  git_commit *commit = nullptr;
  const git_blame_hunk *hunk = git_blame_get_hunk_byindex(d.data(), index);
  git_commit_lookup(&commit, repo, &hunk->final_commit_id);
  return commit ? Commit(commit).message(Commit::SubstituteEmoji) : QString();
}

Signature Blame::signature(int index) const {
  return git_blame_get_hunk_byindex(d.data(), index)->final_signature;
}

bool Blame::isCommitted(int index) const {
  const git_blame_hunk *hunk = git_blame_get_hunk_byindex(d.data(), index);
  return !git_oid_is_zero(&hunk->final_commit_id);
}

Blame Blame::updated(const QByteArray &buffer) const {
  git_blame *blame = nullptr;
  git_blame_buffer(&blame, d.data(), buffer, buffer.length());
  return Blame(blame, repo);
}

} // namespace git
