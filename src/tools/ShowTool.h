//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SHOWTOOL_H
#define SHOWTOOL_H

#include "ExternalTool.h"

class ShowTool : public ExternalTool {
  Q_OBJECT

public:
  static bool openFileManager(QString path);

  ShowTool(const QString &file, QObject *parent = nullptr);

  Kind kind() const override;
  QString name() const override;

  bool start() override;
};

#endif
