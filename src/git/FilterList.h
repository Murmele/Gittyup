//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef FILTERLIST_H
#define FILTERLIST_H

#include "git2/filter.h"
#include <QSharedPointer>

namespace git {

class FilterList {
public:
  bool isValid() const { return !d.isNull(); }

private:
  FilterList(git_filter_list *filter = nullptr);
  operator git_filter_list *() const;

  QSharedPointer<git_filter_list> d;

  friend class Patch;
  friend class Repository;
};

} // namespace git

#endif
