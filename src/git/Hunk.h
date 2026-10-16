//
//          Copyright (c) 2026
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Anand Hegde
//

#ifndef HUNK_H
#define HUNK_H

#include "Id.h"
#include "Signature.h"
#include "git2/blame.h"

namespace git {

/*!
 * \brief Wraps a single blame hunk.
 *
 * The hunk is owned by the git_blame it came from, so this is a non-owning
 * view. libgit2 hands back a null hunk for commits it cannot describe (for
 * instance when the author has no email address), so an invalid Hunk is a
 * normal result and every accessor is safe to call on one.
 */
class Hunk {
public:
  Hunk(const git_blame_hunk *hunk = nullptr);

  bool isValid() const { return d; }
  explicit operator bool() const { return isValid(); }

  int line() const;
  Id id() const;
  Signature signature() const;

  bool isCommitted() const;

private:
  const git_blame_hunk *d;
};

} // namespace git

#endif
