#ifndef BUTTON_H
#define BUTTON_H

#include "Button.h"
#include <QPainterPath>

class DisclosureButton : public Button {
public:
  DisclosureButton(QWidget *parent = nullptr) : Button(parent) {
    setCheckable(true);
    setChecked(true);
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    QPainter painter(this);
    initButtonPainter(&painter);

    qreal x = width() / 2;
    qreal y = height() / 2;

    QPainterPath path;
    if (isChecked()) {
      path.moveTo(x - 3.5, y - 2);
      path.lineTo(x, y + 2);
      path.lineTo(x + 3.5, y - 2);
    } else {
      path.moveTo(x - 2, y + 3.5);
      path.lineTo(x + 2, y);
      path.lineTo(x - 2, y - 3.5);
    }

    painter.drawPath(path);
  }
};

#endif // BUTTON_H
