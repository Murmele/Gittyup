//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Footer.h"
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QStyleOption>
#include <QStylePainter>
#include <QToolButton>
#include <QtMath>

namespace {

class FooterButton : public QToolButton {
public:
  enum Kind { Plus, Minus, Gear };

  FooterButton(Kind kind, QWidget *parent = nullptr)
      : QToolButton(parent), mKind(kind) {}

  QSize sizeHint() const override { return QSize(24, 20); }

protected:
  void paintEvent(QPaintEvent *event) override {
    // Turn off menu arrow.
    QStylePainter sp(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    opt.features &= ~QStyleOptionToolButton::HasMenu;
    sp.drawComplexControl(QStyle::CC_ToolButton, opt);

    // center
    qreal x = width() / 2;
    qreal y = height() / 2;

    // radii
    qreal inner = 3;
    qreal outer = 5;

    QPainterPath path;
    switch (mKind) {
      case Plus:
        path.moveTo(x, y - inner);
        path.lineTo(x, y + inner);
        path.moveTo(x - inner, y);
        path.lineTo(x + inner, y);
        break;

      case Minus:
        path.moveTo(x - inner, y);
        path.lineTo(x + inner, y);
        break;

      case Gear:
        path.addEllipse(QPointF(x, y), inner, inner);
        for (int i = 0; i < 8; ++i) {
          qreal angle = i * M_PI_4; // in radians
          path.moveTo(x + qCos(angle) * inner, y + qSin(angle) * inner);
          path.lineTo(x + qCos(angle) * outer, y + qSin(angle) * outer);
        }
        break;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor color = opt.palette.buttonText().color();
    painter.setPen(QPen(color, 2));
    painter.drawPath(path);
  }

private:
  Kind mKind;
};

} // namespace

Footer::Footer(QWidget *parent) : QWidget(parent) {
  mPlus = new FooterButton(FooterButton::Plus, this);
  connect(mPlus, &FooterButton::clicked, this, &Footer::plusClicked);

  mMinus = new FooterButton(FooterButton::Minus, this);
  mMinus->setEnabled(false);
  connect(mMinus, &FooterButton::clicked, this, &Footer::minusClicked);

  mGear = new FooterButton(FooterButton::Gear, this);
  mGear->setPopupMode(QToolButton::InstantPopup);
  mGear->setVisible(false);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(mPlus);
  layout->addWidget(mMinus);
  layout->addWidget(mGear);
  layout->addStretch();
}

void Footer::setPlusMenu(QMenu *menu) {
  delete mPlus->menu();
  mPlus->setMenu(menu);
  mPlus->setPopupMode(QToolButton::InstantPopup);
}

void Footer::setContextMenu(QMenu *menu) {
  delete mGear->menu();
  mGear->setMenu(menu);
  mGear->setVisible(menu);
}

void Footer::setPlusEnabled(bool enabled) { mPlus->setEnabled(enabled); }

void Footer::setMinusEnabled(bool enabled) { mMinus->setEnabled(enabled); }

void Footer::paintEvent(QPaintEvent *event) {
  QStyleOption option;
  option.initFrom(this);
  QPainter painter(this);
  style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}
