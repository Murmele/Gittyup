//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#ifndef ADDREMOTEDIALOG_H
#define ADDREMOTEDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;

class AddRemoteDialog : public QDialog {
  Q_OBJECT

public:
  AddRemoteDialog(const QString &name = QString(), QWidget *parent = nullptr);

  QString name() const;
  QString url() const;

private:
  void update(const QString &text = QString());

  QLineEdit *mName;
  QLineEdit *mUrl;
  QPushButton *mAdd;
};

#endif
