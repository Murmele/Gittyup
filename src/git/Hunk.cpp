//
//          Copyright (c) 2026
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Anand Hegde
//

#include "Hunk.h"
#include "git2/oid.h"

namespace git {

Hunk::Hunk(const git_blame_hunk *hunk) : d(hunk) {}

int Hunk::line() const { return d ? d->final_start_line_number : -1; }

Id Hunk::id() const { return d ? Id(d->final_commit_id) : Id(); }

Signature Hunk::signature() const {
  return d ? Signature(d->final_signature) : Signature();
}

bool Hunk::isCommitted() const {
  return d && !git_oid_is_zero(&d->final_commit_id);
}

} // namespace git
