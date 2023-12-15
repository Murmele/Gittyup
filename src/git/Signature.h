//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SIGNATURE_H
#define SIGNATURE_H

#include "git2/signature.h"
#include <QSharedPointer>

struct git_signature;
class QDateTime;
class QString;

namespace git {

class Signature {
public:
  bool isValid() const { return d ? true : false; }
  explicit operator bool() const { return isValid(); }

  QString name() const;
  QString email() const;
  QDateTime date() const;
  git_time gitDate() const;

  // Calculate initials.
  QString initials() const;
  static QString initials(const QString &name);

private:
  Signature(git_signature *signature = nullptr, bool owned = false);
  Signature(const QString &name, const QString &email);
  Signature(const QString &name, const QString &email, const QDateTime &date);
  operator const git_signature *() const;

  QSharedPointer<git_signature> d;

  friend class Blame;
  friend class Commit;
  friend class Rebase;
  friend class Repository;
  friend class Tag;
};

} // namespace git

#endif
