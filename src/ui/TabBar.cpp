//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TabBar.h"

TabBar::TabBar(QWidget *parent) : QTabBar(parent) {
  setAutoHide(true);
  setDocumentMode(true);
}

QSize TabBar::minimumTabSizeHint(int index) const {
  mCalculatingMinimumSize = true;
  QSize size = QTabBar::minimumTabSizeHint(index);
  mCalculatingMinimumSize = false;

  // Default Tab just a small tab size on the left
  return size;
}

QSize TabBar::tabSizeHint(int index) const {
  if (!count() || mCalculatingMinimumSize)
    return QTabBar::tabSizeHint(index);

  int height = fontMetrics().lineSpacing() + 12;
  return QSize(parentWidget()->width() / count() + 1, height);

  // Default Tab just a small tab size on the left
}
