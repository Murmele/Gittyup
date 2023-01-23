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

bool Cache::get(const QString &url, QString &username, QString &password) {
  if (!mCache.contains(url))
    return false;

  const QMap<QString, QString> &map = mCache[url];
  if (map.isEmpty())
    return false;

  if (username.isEmpty())
    username = map.keys().first();

  if (!map.contains(username))
    return false;

  password = map.value(username);
  return true;
}

bool Cache::store(const QString &url, const QString &username,
                  const QString &password) {
  mCache[url][username] = password;
  return true;
}
