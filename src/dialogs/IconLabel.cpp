//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "IconLabel.h"
#include <QPainter>
#include <QStyle>

IconLabel::IconLabel(const QIcon &icon, int width, int height, QWidget *parent)
    : QWidget(parent), mIcon(icon), mWidth(width), mHeight(height) {
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

QSize IconLabel::sizeHint() const { return QSize(mWidth, mHeight); }

QSize IconLabel::minimumSizeHint() const { return sizeHint(); }

void IconLabel::paintEvent(QPaintEvent *event) {
  QSize size = sizeHint();
  QPainter(this).drawPixmap(
      QStyle::alignedRect(Qt::LeftToRight, Qt::AlignHCenter, size, rect()),
      mIcon.pixmap(size, window()->devicePixelRatio()));
}
