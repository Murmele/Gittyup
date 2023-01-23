//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BUFFER_H
#define BUFFER_H

#include "git2/buffer.h"

namespace git {

class Buffer {
public:
  Buffer(const char *data, int size);

  bool isBinary() const;

private:
  git_buf d;
};

} // namespace git

#endif
