//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Installer.h"
#include <QDir>

Installer::Installer(const QString &name, const QString &path)
    : mName(name), mPath(path) {}

bool Installer::exists() const {
  return (!mName.isEmpty() && !mPath.isEmpty() && QDir(mPath).exists(mName));
}
