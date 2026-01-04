//
//          Copyright (c) 2022, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Hessamoddin Hediehloo(H-4ND-H)
//

#ifndef STORE_H
#define STORE_H

#include "CredentialHelper.h"
#include <QMap>

/*!
 * \brief The Store class
 * git-credential-store
 * https://git-scm.com/docs/git-credential-store
 * Storing the credentials encoded but unencrypted on disk
 */
class Store : public CredentialHelper {
public:
  Store(const QString &path);

  bool get(const QString &url, QString &username, QString &password) override;

  bool store(const QString &url, const QString &username,
             const QString &password) override;

private:
  QString command() const;
  QMap<QString, QMap<QString, QMap<QString, QString>>> readCredFile();
  bool extractUserPass(const QMap<QString, QString> &map, QString &username,
                       QString &password);

  QString mPath;
};

#endif
