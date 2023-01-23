//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TAG_H
#define TAG_H

#include "Object.h"
#include "git2/tag.h"

namespace git {

class Signature;

class Tag : public Object {
public:
  Tag();
  Tag(const Object &rhs);

  Signature tagger() const;
  QString message() const;

private:
  Tag(git_tag *tag);
  operator git_tag *() const;

  friend class TagRef;
};

} // namespace git

#endif
