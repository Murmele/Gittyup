//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Tag.h"
#include "Signature.h"

namespace git {

Tag::Tag() : Object() {}

Tag::Tag(const Object &rhs) : Object(rhs) {
  if (isValid() && type() != GIT_OBJECT_TAG)
    d.clear();
}

Tag::Tag(git_tag *tag) : Object(reinterpret_cast<git_object *>(tag)) {}

Tag::operator git_tag *() const {
  return reinterpret_cast<git_tag *>(d.data());
}

Signature Tag::tagger() const {
  return const_cast<git_signature *>(git_tag_tagger(*this));
}

QString Tag::message() const {
  // FIXME: Tag doesn't remember its encoding?
  return git_tag_message(*this);
}

} // namespace git
