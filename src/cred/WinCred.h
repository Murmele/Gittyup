//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef WINCRED_H
#define WINCRED_H

#include "CredentialHelper.h"

class WinCred : public CredentialHelper {
public:
  WinCred();

  bool get(const QString &url, QString &username, QString &password) override;

  bool store(const QString &url, const QString &username,
             const QString &password) override;
};

#endif
