//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

class QTabBar;

class AboutDialog : public QDialog {
  Q_OBJECT

public:
  enum Index { Changelog, Acknowledgments, Privacy };

  AboutDialog(QWidget *parent = nullptr);

  static void openSharedInstance(Index index = Changelog);

private:
  void setCurrentIndex(Index index);

  QTabBar *mTabs;
};

#endif
