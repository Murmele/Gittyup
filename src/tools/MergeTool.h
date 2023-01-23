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

#include "ExternalTool.h"
#include "git/Blob.h"

class MergeTool : public ExternalTool {
  Q_OBJECT

public:
  MergeTool(const QString &file, const git::Blob &localBlob,
            const git::Blob &remoteBlob, const git::Blob &baseBlob,
            QObject *parent = nullptr);

  bool isValid() const override;

  Kind kind() const override;
  QString name() const override;

  bool start() override;

protected:
  git::Blob mLocalBlob;
  git::Blob mRemoteBlob;
  git::Blob mBaseBlob;
};

#endif
