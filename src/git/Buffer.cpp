//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Buffer.h"
#include "git2/blob.h"

namespace git {

Buffer::Buffer(const char *data, int size) : data(data), size(size) {}

bool Buffer::isBinary() const { return git_blob_data_is_binary(data, size); }

} // namespace git
