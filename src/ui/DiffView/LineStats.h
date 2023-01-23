#ifndef LINESTATS_H
#define LINESTATS_H

#include <QWidget>
#include <QPainter>

#include "git/Patch.h"
#include "app/Application.h"

class LineStats : public QWidget {
public:
  LineStats(const git::Patch::LineStats &stats, QWidget *parent = nullptr)
      : QWidget(parent) {
    setStats(stats);
  }

  void setStats(const git::Patch::LineStats &stats) {
    qreal x = static_cast<qreal>(stats.additions + stats.deletions) / 5;
    if (x > 0) {
      bool scaled = (stats.additions + stats.deletions > 5);
      mPluses = scaled ? stats.additions / x : stats.additions;
      mMinuses = scaled ? stats.deletions / x : stats.deletions;
    } else {
      mPluses = 0;
      mMinuses = 0;
    }
  }

  QSize minimumSizeHint() const override {
    return QSize(10 * (mPluses + mMinuses), 12);
  }

  void paintEvent(QPaintEvent *event) override {
    // Drawing the outline of the shapes with a narrow pen approximates
    // a slight drop shadow. Minus has to be drawn slightly smaller than
    // plus or it creates an optical illusion that makes it look to big.

    QPainter painter(this);
    qreal w = width() / (mPluses + mMinuses);
    qreal h = height();
    qreal x = w / 2;
    qreal y = h / 2;

    painter.setPen(QPen(Qt::black, 0.1));
    painter.setBrush(Application::theme()->diff(Theme::Diff::Plus));
    for (int i = 0; i < mPluses; ++i) {
      QPainterPath path;
      path.moveTo(x - 4, y - 1);
      path.lineTo(x - 1, y - 1);
      path.lineTo(x - 1, y - 4);
      path.lineTo(x + 1, y - 4);
      path.lineTo(x + 1, y - 1);
      path.lineTo(x + 4, y - 1);
      path.lineTo(x + 4, y + 1);
      path.lineTo(x + 1, y + 1);
      path.lineTo(x + 1, y + 4);
      path.lineTo(x - 1, y + 4);
      path.lineTo(x - 1, y + 1);
      path.lineTo(x - 4, y + 1);
      path.closeSubpath();
      painter.drawPath(path);
      x += w;
    }

    painter.setBrush(Application::theme()->diff(Theme::Diff::Minus));
    for (int i = 0; i < mMinuses; ++i) {
      QPainterPath path;
      path.moveTo(x - 3, y - 1);
      path.lineTo(x + 4, y - 1);
      path.lineTo(x + 4, y + 1);
      path.lineTo(x - 3, y + 1);
      path.closeSubpath();
      painter.drawPath(path);
      x += w;
    }
  }

private:
  int mPluses = 0;
  int mMinuses = 0;
};

#endif // LINESTATS_H
