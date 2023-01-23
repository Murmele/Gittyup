//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef INSTALLER_H
#define INSTALLER_H

#include <QString>

class Installer {
public:
  Installer(const QString &name, const QString &path);

  bool exists() const;
  bool isInstalled() const;

  bool install();
  bool uninstall();

private:
  QString mName;
  QString mPath;
};

#endif
