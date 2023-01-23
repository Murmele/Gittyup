//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <QDialog>

class QLabel;

class UpdateDialog : public QDialog {
  Q_OBJECT

public:
  UpdateDialog(const QString &platform, const QString &version,
               const QString &changelog, const QString &link,
               QWidget *parent = nullptr);
};

#endif
