//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "FilterList.h"

namespace git {

FilterList::FilterList(git_filter_list *filter)
    : d(filter, git_filter_list_free) {}

FilterList::operator git_filter_list *() const { return d.data(); }

} // namespace git
