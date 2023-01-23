//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Installer.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>

bool Installer::isInstalled() const {
  QString path = QCoreApplication::applicationFilePath();
  return (QFile::symLinkTarget(QDir(mPath).filePath(mName)) == path);
}

bool Installer::install() {
  QDir dir(mPath);
  if (!dir.exists() && !dir.mkpath(".")) {
    QProcess proc;
    proc.start("/usr/bin/pkexec", {"/bin/mkdir", "-p", dir.path()});
    proc.waitForFinished();
  }

  if (!dir.exists())
    return false;

  QString target = dir.filePath(mName);
  QString source = QCoreApplication::applicationFilePath();
  if (QFile::link(source, target))
    return true;

  QProcess process;
  process.start("/usr/bin/pkexec", {"/bin/ln", "-s", source, target});
  process.waitForFinished();
  return true;
}

bool Installer::uninstall() {
  QDir dir(mPath);
  if (dir.remove(mName))
    return true;

  QProcess process;
  process.start("/usr/bin/pkexec", {"/bin/rm", dir.filePath(mName)});
  process.waitForFinished();
  return true;
}
