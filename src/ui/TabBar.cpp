//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TabBar.h"
#include <QMouseEvent>
#include <QDebug>

namespace {
constexpr auto home_tab_width = 50;
}

TabBar::TabBar(QWidget *parent) : QTabBar(parent) {
  setAutoHide(true);
  setDocumentMode(true);
}

void TabBar::mousePressEvent(QMouseEvent* event) {
  mClickedTabIndex = tabAt(event->pos());
  QTabBar::mousePressEvent(event);
}

void TabBar::mouseMoveEvent(QMouseEvent* event) {
  qDebug() << event->pos();
  // if (mClickedTabIndex == 0/*event->pos().x() <= home_tab_width || tabAt(event->pos()) == 0*/) { //
  //   // Ignoring first tab because this is the welcome tab
  //   return;
  // }

  // QTabBar::mouseMoveEvent(event);
  return;
}

void TabBar::mouseReleaseEvent(QMouseEvent* event) {
  mClickedTabIndex = -1;
  QTabBar::mouseMoveEvent(event);
}

QSize TabBar::minimumTabSizeHint(int index) const {
  mCalculatingMinimumSize = true;
  QSize size = QTabBar::minimumTabSizeHint(index);
  mCalculatingMinimumSize = false;

  if (index == 0)
    size.setWidth(home_tab_width);

  // Default Tab just a small tab size on the left
  return size;
}

QSize TabBar::tabSizeHint(int index) const {
  if (!count() || mCalculatingMinimumSize)
    return QTabBar::tabSizeHint(index);

  int height = fontMetrics().lineSpacing() + 12;
  if (index == 0)
    return QSize(home_tab_width, height);
  return QSize(parentWidget()->width() / (count() - 1) + 1 - home_tab_width, height);

  // Default Tab just a small tab size on the left
}
