//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Signature.h"
#include "git2/signature.h"
#include <QDateTime>

namespace git {

Signature::Signature(git_signature *signature, bool owned)
    : d(
          signature, owned ? git_signature_free : [](git_signature *) {}) {}

Signature::Signature(const QString &name, const QString &email) {
  git_signature *signature = nullptr;

  git_signature_now(&signature, name.toUtf8(), email.toUtf8());
  d = QSharedPointer<git_signature>(signature, git_signature_free);
}

Signature::Signature(const QString &name, const QString &email,
                     const QDateTime &date) {
  git_signature *signature = nullptr;

  auto offset = date.offsetFromUtc() / 60;
  git_signature_new(&signature, name.toUtf8(), email.toUtf8(),
                    date.toSecsSinceEpoch(), offset);
  d = QSharedPointer<git_signature>(signature, git_signature_free);
}

Signature::operator const git_signature *() const { return d.data(); }

QString Signature::name() const { return d->name; }

QString Signature::email() const { return d->email; }

QDateTime Signature::date() const {
  int offset = d->when.offset * 60; // Convert from minutes to seconds.
  return QDateTime::fromTime_t(d->when.time, Qt::OffsetFromUTC, offset);
}

git_time Signature::gitDate() const { return d->when; }

QString Signature::initials() const { return initials(name()); }

QString Signature::initials(const QString &name) {
  QStringList names = name.split(' ');
  QStringList initials(names.first().left(1).toUpper());
  if (names.size() > 1)
    initials.append(names.last().left(1).toUpper());
  return initials.join(QString());
}

} // namespace git
