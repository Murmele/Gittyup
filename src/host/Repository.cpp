//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Repository.h"
#include "Account.h"

Repository::Repository(const QString &name, const QString &fullName,
                       Account *parent)
    : QObject(parent), mName(name), mFullName(fullName) {}

Account *Repository::account() const {
  return static_cast<Account *>(parent());
}
