//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SearchField.h"
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QToolButton>

namespace {

const QString kSearchFieldStyle = "QToolButton {"
                                  "  border: none"
                                  "}"
                                  "QToolButton:!pressed {"
                                  "  background: none"
                                  "}"
                                  "QLineEdit {"
                                  "  border-radius: 4px;"
                                  "  padding: 0px %1px 0px 4px"
                                  "}";

class Button : public QToolButton {
public:
  Button(QWidget *parent = nullptr) : QToolButton(parent) {
    setCursor(Qt::ArrowCursor);
  }

  QSize sizeHint() const override { return QSize(16, 16); }
};

class AdvancedButton : public Button {
  Q_OBJECT

public:
  AdvancedButton(QWidget *parent = nullptr) : Button(parent) {
    setToolTip(tr("Advanced Search"));
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    QToolButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(palette().color(QPalette::Text), 1.5));

    QPainterPath path;
    path.moveTo(-3, -1.5);
    path.lineTo(0, 1.5);
    path.lineTo(3, -1.5);

    painter.drawPath(path.translated(width() / 2, height() / 2));
  }
};

class ClearButton : public Button {
  Q_OBJECT

public:
  ClearButton(QWidget *parent = nullptr) : Button(parent) {
    setToolTip(tr("Clear"));
  }

protected:
  void paintEvent(QPaintEvent *event) override {
    QToolButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(palette().color(QPalette::Text), 1.5));

    QPainterPath path;
    path.moveTo(-3, -3);
    path.lineTo(3, 3);
    path.moveTo(3, -3);
    path.lineTo(-3, 3);

    painter.drawPath(path.translated(width() / 2, height() / 2));
  }
};

} // namespace

SearchField::SearchField(QWidget *parent) : QLineEdit(parent) {
  setPlaceholderText(tr("Search"));
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  mClearButton = new ClearButton(this);
  mClearButton->hide();

  connect(mClearButton, &QToolButton::clicked, this, &SearchField::clear);
  connect(this, &SearchField::textChanged, [this](const QString &text) {
    mClearButton->setVisible(!text.isEmpty() && isEnabled());
  });

  mAdvancedButton = new AdvancedButton(this);

  int cbw = mClearButton->sizeHint().width();
  int abw = mAdvancedButton->sizeHint().width();
  int frame = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(kSearchFieldStyle.arg(cbw + abw + frame + 2));
}

QSize SearchField::sizeHint() const {
  return QSize(1.5 * QLineEdit::sizeHint().width(), 24);
}

void SearchField::resizeEvent(QResizeEvent *event) {
  // This assumes that both buttons have the same size.
  int frame = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  QSize size = mAdvancedButton->sizeHint();
  int x = rect().right() - frame - 2;
  int y = (height() - size.height()) / 2.0;
  mAdvancedButton->move(x - size.width(), y);
  mClearButton->move(x - (2 * size.width()), y);
}

#include "SearchField.moc"
