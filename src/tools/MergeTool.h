//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef MERGETOOL_H
#define MERGETOOL_H

#include <QString>
#include <QVector>
#include "ExternalTool.h"
#include "git/Blob.h"

class QObject;
namespace git {
  class Diff;
  class Repository;
};

class MergeTool : public ExternalTool {
  Q_OBJECT

public:
  MergeTool(const QStringList &files, const git::Diff &diff,
            const git::Repository &repo, QObject *parent);

  bool isValid() const override;

  Kind kind() const override;
  QString name() const override;

  bool start() override;

protected:

private:
  struct FileMerge {
    QString name;
    git::Blob local;
    git::Blob remote;
    git::Blob base;
  };
  QVector<FileMerge> mMerges;
};

#endif
