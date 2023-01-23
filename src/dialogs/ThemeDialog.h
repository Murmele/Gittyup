//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#ifndef THEMEDIALOG_H
#define THEMEDIALOG_H

#include <QDialog>

class ThemeDialog : public QDialog {
  Q_OBJECT

public:
  ThemeDialog(QWidget *parent = nullptr);
};

#endif
