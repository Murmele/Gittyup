//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Buffer.h"
#include "git2/deprecated.h"

namespace git {

Buffer::Buffer(const char *data, int size)
  : d(GIT_BUF_INIT_CONST(data, size))
{}

bool Buffer::isBinary() const
{
  return git_buf_is_binary(&d);
}

} // namespace git
