//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CONFFILE_H
#define CONFFILE_H

#include <QDir>
#include <QString>
#include <QVariant>

class ConfFile {
public:
  ConfFile(const QString &filename);

  // Parse Lua source held in memory rather than on disk. baseDir is used
  // to set up package.path so the script can still require() files
  // relative to the theme/config directory it logically belongs to.
  ConfFile(const QByteArray &source, const QDir &baseDir);

  virtual ~ConfFile();

  // Table is the name of a new global table that the script
  // is expected to modify. If the table name is empty, then
  // the script should return a single anonymous table.
  QVariantMap parse(const QString &table = QString());

private:
  QString mFilename;
  QByteArray mSource;
  QDir mBaseDir;
};

#endif
