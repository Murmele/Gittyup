//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef RESULT_H
#define RESULT_H

#include <QString>

namespace git {

// Capture the error string associated with an error immediately after
// it's generated. This is especially important for returning a result
// to another thread since the last error is thread-specific data.
class Result {
public:
  Result(int error = -1);
  Result(int error, const QString &errorString)
      : mError(error), mErrorString(errorString) {}

  int error() const { return mError; }
  explicit operator bool() const { return !mError; }

  QString errorString(const QString &defaultError = QString()) const;

private:
  int mError = -1;
  QString mErrorString;
};

} // namespace git

#endif
