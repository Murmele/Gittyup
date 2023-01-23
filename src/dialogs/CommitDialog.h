//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef COMMITDIALOG_H
#define COMMITDIALOG_H

#include "conf/Setting.h"
#include <QDialog>

class QTextEdit;

class CommitDialog : public QDialog {
  Q_OBJECT

public:
  CommitDialog(const QString &message, Prompt::Kind kind,
               QWidget *parent = nullptr);

  QString message() const;

  void open() override;

private:
  QTextEdit *mEditor;
};

#endif
