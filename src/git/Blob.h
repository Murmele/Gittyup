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

  bool isBinary() const;
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
