//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Repository.h"
#include "TagRef.h"
#include "Tag.h"

namespace git {

TagRef::TagRef(git_reference *ref) : Reference(ref) {
  if (isValid() && !isTag())
    d.clear();
}

TagRef::TagRef(const Reference &rhs) : Reference(rhs) {
  if (isValid() && !isTag())
    d.clear();
}

Tag TagRef::tag() const {
  git_tag *tag = nullptr;
  git_repository *repo = git_reference_owner(d.data());
  git_tag_lookup(&tag, repo, git_reference_target(d.data()));
  return Tag(tag);
}

bool TagRef::remove() {
  // Remember name.
  QString name = this->name();

  Repository repo(git_reference_owner(d.data()));
  emit repo.notifier()->referenceAboutToBeRemoved(*this);

  int error = git_reference_delete(d.data());
  if (!error)
    d.clear(); // Invalidate this branch.

  // We have to notify even if removal failed and the tag is still valid.
  // Clients can check this tag to see if the tag was really removed.
  emit repo.notifier()->referenceRemoved(name);

  return !error;
}

} // namespace git
