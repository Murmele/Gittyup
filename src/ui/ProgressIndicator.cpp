//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ProgressIndicator.h"
#include <QCursor>
#include <QPainter>
#include <QtMath>

namespace {

// size
const int kSize = 26;

} // namespace

QSize ProgressIndicator::size() { return QSize(kSize, kSize); }

void ProgressIndicator::paint(QPainter *painter, const QRect &rect,
                              const QColor &c, int progress,
                              const QWidget *widget) {
  painter->save();
  painter->setRenderHints(QPainter::Antialiasing);

  // center
  qreal x = rect.x() + (rect.width() / 2);
  qreal y = rect.y() + (rect.height() / 2);

  if (widget && rect.contains(widget->mapFromGlobal(QCursor::pos()))) {
    // radii
    const qreal in = 3;
    const qreal out = 8;

    // Draw background.
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#BEBEBE"));
    painter->drawEllipse(QPointF(x, y), out, out);

    // Draw x.
    painter->setPen(QPen(QColor("#646464"), 1.5));
    painter->drawLine(QPointF(x - in, y - in), QPointF(x + in, y + in));
    painter->drawLine(QPointF(x - in, y + in), QPointF(x + in, y - in));

  } else {
    // radii
    const qreal in = 7;
    const qreal out = 12;

    int alpha = 32;
    QColor color = c;
    for (int i = 0; i < 12; ++i) {
      color.setAlpha(alpha);
      alpha += 16;

      painter->setPen(QPen(color, 2.5, Qt::SolidLine, Qt::RoundCap));

      qreal angle = (i + (progress % 12)) * (M_PI / 6); // in radians
      painter->drawLine(QLineF(x + qCos(angle) * in, y + qSin(angle) * in,
                               x + qCos(angle) * out, y + qSin(angle) * out));
    }
  }

  painter->restore();
}
