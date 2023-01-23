//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TAGREF_H
#define TAGREF_H

#include "Reference.h"

namespace git {

class Tag;

class TagRef : public Reference {
public:
  TagRef(const Reference &rhs);

  Tag tag() const;

  bool remove();

private:
  TagRef(git_reference *ref = nullptr);

  friend class Repository;
};

} // namespace git

#endif
