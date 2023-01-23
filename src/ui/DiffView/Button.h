#ifndef DIFFVIEW_BUTTON_H
#define DIFFVIEW_BUTTON_H

#include <QToolButton>
#include <QStyle>
#include <QStyleOptionToolButton>
#include <QPainter>

class Button : public QToolButton {
public:
  Button(QWidget *parent = nullptr) : QToolButton(parent) {}

  QSize sizeHint() const override {
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    // Start with the check box dimensions.
    int width = style()->pixelMetric(QStyle::PM_IndicatorWidth, &opt, this);
    int height = style()->pixelMetric(QStyle::PM_IndicatorHeight, &opt, this);
    return QSize(width + 2, height + 4);
  }

protected:
  void initButtonPainter(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen = painter->pen();
    pen.setWidth(2);
    painter->setPen(pen);
  }
};

#endif // DIFFVIEW_BUTTON_H
