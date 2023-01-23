//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ContextMenuButton.h"
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>
#include <QStylePainter>
#include <QToolBar>
#include <QtMath>

namespace {

const QString kStyleSheet = "QToolButton {"
                            "  border: none;"
                            "  border-radius: 4px;"
                            "  padding: 0px 4px 0px 4px"
                            "}";

} // namespace

ContextMenuButton::ContextMenuButton(QWidget *parent) : QToolButton(parent) {
  setStyleSheet(kStyleSheet);
  setPopupMode(QToolButton::InstantPopup);
}

QSize ContextMenuButton::sizeHint() const { return QSize(24, 18); }

void ContextMenuButton::paintEvent(QPaintEvent *event) {
  QStylePainter sp(this);
  QStyleOptionToolButton opt;
  initStyleOption(&opt);
  opt.features &= ~QStyleOptionToolButton::HasMenu;
  sp.drawComplexControl(QStyle::CC_ToolButton, opt);

  // FIXME: This is an egregious hack that just happens to work for both
  // themes. The difficulty is that the 'pressed' button text color is set
  // in the style sheet and there's no obvious way to get the palette that
  // the style sheet style uses. I haven't had any luck hooking into the
  // button drawing process with a proxy style either.
  QToolBar *parent = qobject_cast<QToolBar *>(parentWidget());
  QBrush brush = (parent && isDown()) ? Qt::white : opt.palette.buttonText();

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(QPen(brush, 2.25));

  qreal x = width() / 2.0;
  qreal y = height() / 2.0;

  // radii
  qreal inner = 4;
  qreal outer = 6.5;

  QPainterPath path;
  path.addEllipse(QPointF(x, y), inner, inner);
  for (int i = 0; i < 8; ++i) {
    qreal angle = i * M_PI_4; // in radians
    path.moveTo(x + qCos(angle) * inner, y + qSin(angle) * inner);
    path.lineTo(x + qCos(angle) * outer, y + qSin(angle) * outer);
  }

  painter.drawPath(path);
}
