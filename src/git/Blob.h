//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef BLOB_H
#define BLOB_H

#include "Object.h"
#include "git2/blob.h"

namespace git {

class Blob : public Object {
public:
  Blob();
  Blob(const Object &rhs);

  /// @brief Check if the Blob object points to binary data
  /// @return True if Blob oject is for a binary blob
  bool isBinary() const;

  /// @brief Check if given QByteArray contains binary data according to libgit2
  /// @param data QByteArray to do a binary test on
  /// @return True if data is binary
  static bool isBinary(const QByteArray &data);

  /// @brief Grab the content of the blob
  /// @return QByteArray of the blob
  QByteArray content() const;

private:
  Blob(git_blob *blob);
  operator git_blob *() const;

  friend class Diff;
  friend class Patch;
  friend class Repository;
};

} // namespace git

Q_DECLARE_METATYPE(git::Blob);

#endif
