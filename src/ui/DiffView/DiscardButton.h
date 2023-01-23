#ifndef DISCARDBUTTON_H
#define DISCARDBUTTON_H

#include "Button.h"

class DiscardButton : public Button {
public:
  DiscardButton(QWidget *parent = nullptr) : Button(parent) {
    setObjectName("DiscardButton");
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    QPainter painter(this);
    initButtonPainter(&painter);

    qreal r = 4;
    qreal x = width() / 2;
    qreal y = height() / 2;

    QPainterPath path;
    path.moveTo(x - r, y - r);
    path.lineTo(x + r, y + r);
    path.moveTo(x + r, y - r);
    path.lineTo(x - r, y + r);
    painter.drawPath(path);
  }
};

#endif // DISCARDBUTTON_H
