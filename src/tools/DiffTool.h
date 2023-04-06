//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFFTOOL_H
#define DIFFTOOL_H

#include <QString>
#include "ExternalTool.h"

class QObject;
namespace git {
class Diff;
class Repository;
class Blob;
};

class DiffTool : public ExternalTool {
  Q_OBJECT

public:
  DiffTool(const QStringList &files, const git::Diff &diff,
           const git::Repository &repo, QObject *parent);

  bool isValid() const override;

  Kind kind() const override;
  QString name() const override;

  bool start() override;

protected:

private:
  bool getBlob(const QString &file, const git::Diff::File &version,
               git::Blob &blob) const;
};

#endif
