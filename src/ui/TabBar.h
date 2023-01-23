//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>

class TabBar : public QTabBar {
  Q_OBJECT

public:
  TabBar(QWidget *parent = nullptr);

protected:
  QSize minimumTabSizeHint(int index) const override;
  QSize tabSizeHint(int index) const override;

private:
  mutable bool mCalculatingMinimumSize = false;
};

#endif
