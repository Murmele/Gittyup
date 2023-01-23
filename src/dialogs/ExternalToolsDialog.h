//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#ifndef EXTERNALTOOLSDIALOG_H
#define EXTERNALTOOLSDIALOG_H

#include <QDialog>

class ExternalToolsModel;
class QVBoxLayout;

class ExternalToolsDialog : public QDialog {
  Q_OBJECT

public:
  ExternalToolsDialog(const QString &type, QWidget *parent = nullptr);

private:
  QVBoxLayout *createDetectedLayout(const QString &type);
  QVBoxLayout *createUserDefinedLayout(const QString &type);
};

#endif
