//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef EDITTOOL_H
#define EDITTOOL_H

#include "ExternalTool.h"

class EditTool : public ExternalTool {
  Q_OBJECT

public:
  EditTool(const QString &file, QObject *parent = nullptr);

  bool isValid() const override;

  Kind kind() const override;
  QString name() const override;

  bool start() override;
};

#endif
