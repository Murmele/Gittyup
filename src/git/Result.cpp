//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Result.h"
#include "git2/errors.h"

namespace git {

Result::Result(int error) : mError(error) {
  const git_error *err = git_error_last();
  mErrorString = err ? err->message : QString();
}

QString Result::errorString(const QString &defaultError) const {
  return !mErrorString.isEmpty() ? mErrorString : defaultError;
}

} // namespace git
