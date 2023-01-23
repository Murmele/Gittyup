//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CACHE_H
#define CACHE_H

#include "CredentialHelper.h"
#include <QMap>

class Cache : public CredentialHelper {
public:
  Cache();

  bool get(const QString &url, QString &username, QString &password) override;

  bool store(const QString &url, const QString &username,
             const QString &password) override;

private:
  QMap<QString, QMap<QString, QString>> mCache;
};

#endif
