//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Id.h"
#include <QHash>
#include <cstring>

namespace git {

namespace {

const Id kInvalidId = QByteArray(GIT_OID_SHA1_SIZE, -1);

} // namespace

Id::Id() { memset(d.id, 0, GIT_OID_SHA1_SIZE); }

Id::Id(const QByteArray &id) {
  git_oid_fromraw(&d, reinterpret_cast<const unsigned char *>(id.constData()));
}

Id::Id(const git_oid &id) { git_oid_cpy(&d, &id); }

Id::Id(const git_oid *id) {
  if (id) {
    git_oid_cpy(&d, id);
  } else {
    memset(d.id, 0, GIT_OID_SHA1_SIZE);
  }
}

Id::operator const git_oid *() const { return &d; }

bool Id::isNull() const { return git_oid_is_zero(&d); }

bool Id::isValid() const { return !git_oid_equal(&d, kInvalidId); }

QString Id::toString() const { return toByteArray().toHex(); }

QByteArray Id::toByteArray() const {
  const char *data = reinterpret_cast<const char *>(d.id);
  return QByteArray::fromRawData(data, GIT_OID_SHA1_SIZE);
}

bool Id::operator<(const Id &rhs) const { return (git_oid_cmp(&d, rhs) < 0); }

bool Id::operator==(const Id &rhs) const { return git_oid_equal(&d, rhs); }

bool Id::operator!=(const Id &rhs) const { return !git_oid_equal(&d, rhs); }

Id Id::invalidId() { return kInvalidId; }

uint qHash(const Id &key) { return qHash(key.toByteArray()); }

} // namespace git
