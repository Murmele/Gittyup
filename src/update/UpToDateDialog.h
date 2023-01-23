//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef UPTODATEDDIALOG_H
#define UPTODATEDDIALOG_H

#include <QDialog>

class UpToDateDialog : public QDialog {
  Q_OBJECT

public:
  UpToDateDialog(QWidget *parent = nullptr);
};

#endif
