//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QWidget>

class Account;
class TabWidget;
class MainWindow;

class SideBar : public QWidget {
  Q_OBJECT

public:
  SideBar(TabWidget *tabs, MainWindow *mainWindow, QWidget *parent = nullptr);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

private:
  void promptToRemoveAccount(Account *account);
};

#endif
