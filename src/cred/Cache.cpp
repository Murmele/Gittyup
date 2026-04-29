//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Cache.h"

Cache::Cache() {}

CredentialHelper::Result Cache::get(const QString &url, QString &username,
                                    QString &password) {
  if (!mCache.contains(url))
    return CredentialHelper::Result::ERROR(QStringLiteral(""));

  const QMap<QString, QString> &map = mCache[url];
  if (map.isEmpty())
    return CredentialHelper::Result::ERROR(QStringLiteral(""));

  if (username.isEmpty())
    username = map.keys().first();

  if (!map.contains(username))
    return CredentialHelper::Result::ERROR(QStringLiteral(""));

  password = map.value(username);
  return CredentialHelper::Result::OK();
}

CredentialHelper::Result Cache::store(const QString &url,
                                      const QString &username,
                                      const QString &password) {
  mCache[url][username] = password;
  return CredentialHelper::Result::OK();
}
