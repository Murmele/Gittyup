//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Object.h"
#include "Id.h"
#include "Repository.h"

namespace git {

Object::Object(git_object *obj) : d(obj, git_object_free) {}

Object::operator const git_oid *() const { return git_object_id(d.data()); }

Object::operator const git_object *() const { return d.data(); }

Repository Object::repo() const { return git_object_owner(d.data()); }

git_object_t Object::type() const { return git_object_type(d.data()); }

Id Object::id() const { return git_object_id(d.data()); }

QString Object::shortId() const
{
  git_buf buf = GIT_BUF_INIT;
  git_object_short_id(&buf, d.data());
  QByteArray result(buf.ptr, buf.size);
  git_buf_dispose(&buf);
  return result;
}

QString Object::link() const {
  return QString("<a href='id:%1'>%2</a>").arg(id().toString(), shortId());
}

bool Object::operator==(const Object &rhs) const {
  if (!isValid())
    return !rhs.isValid();

  if (!rhs.isValid())
    return !isValid();

  return (id() == rhs.id());
}

bool Object::operator!=(const Object &rhs) const {
  if (!isValid())
    return rhs.isValid();

  if (!rhs.isValid())
    return isValid();

  return (id() != rhs.id());
}

uint qHash(const Object &key) { return qHash(key.id()); }

} // namespace git
