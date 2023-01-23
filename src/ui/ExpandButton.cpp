//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ExpandButton.h"
#include <QPainter>
#include <QPainterPath>

ExpandButton::ExpandButton(QWidget *parent) : QToolButton(parent) {
  setCheckable(true);
}

QSize ExpandButton::sizeHint() const { return QSize(20, 20); }

void ExpandButton::paintEvent(QPaintEvent *event) {
  QToolButton::paintEvent(event);

  qreal x = width() / 2.0;
  qreal y = height() / 2.0;
  qreal f = isChecked() ? -1 : 1;

  QPainterPath path;
  path.moveTo(x - (3 * f), y - (1.5 * f));
  path.lineTo(x, y + (1.5 * f));
  path.lineTo(x + (3 * f), y - (1.5 * f));

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(painter.pen().color(), 1.5));
  painter.drawPath(path);
}
