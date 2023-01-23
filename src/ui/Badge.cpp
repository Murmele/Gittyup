//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Badge.h"
#include "app/Application.h"
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>

namespace {

const int kSpacing = 4;
const int kPadding = 12;
const int kIconWidth = 14;
const QString kEllipsis = "...";
Qt::Alignment kLabelAlignment = Qt::AlignHCenter | Qt::AlignVCenter;

bool isEmpty(QList<Badge::Label> &labels) {
  if (!labels.isEmpty() && labels.last().text == kEllipsis)
    labels.removeLast();
  return labels.isEmpty();
}

} // namespace

Badge::Badge(const QList<Label> &labels, QWidget *parent)
    : QWidget(parent), mLabels(labels) {
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
}

void Badge::appendLabel(const Label &label) {
  mLabels.append(label);
  updateGeometry();
}

void Badge::setLabels(const QList<Label> &labels) {
  mLabels = labels;
  updateGeometry();
}

QSize Badge::sizeHint() const {
  return !mLabels.isEmpty() ? size(font(), mLabels) : QSize();
}

QSize Badge::minimumSizeHint() const {
  return !mLabels.isEmpty()
             ? size(font(), Label(mLabels.first().type, kEllipsis))
             : QSize();
}

QSize Badge::size(const QFont &font, const QList<Label> &labels) {
  Q_ASSERT(!labels.isEmpty());

  int width = 0;
  QFontMetrics fm(font);
  foreach (const Label &label, labels)
    width += size(font, label).width();
  return QSize(width + ((labels.size() - 1) * kSpacing), fm.lineSpacing() + 2);
}

QSize Badge::size(const QFont &font, const Label &label) {
  QFont copy = font;
  copy.setBold(label.bold);

  QFontMetrics fm(copy);
  int icon = label.tag ? kIconWidth : 0;
  int width = (label.text.length() > 1) ? fm.horizontalAdvance(label.text)
                                        : fm.averageCharWidth();
  return QSize(icon + width + kPadding, fm.lineSpacing() + 2);
}

// Draws the badges in all available space, right-aligned by default.
// Returns the final width occupied by the badges.
int Badge::paint(QPainter *painter, const QList<Label> &list, const QRect &rect,
                 QStyleOption *opt, Qt::Alignment alignment) {
  bool active = (opt && (opt->state & QStyle::State_Active));
  bool selected = (opt && (opt->state & QStyle::State_Selected));

#ifdef Q_OS_WIN
  // Disable selected state in the default theme.
  selected = (selected && Application::theme()->name() != "Default");
#endif

  // Elide labels.
  QList<Label> labels = list;
  QFont font = painter->font();
  while (size(font, labels).width() > rect.width() && !isEmpty(labels))
    labels.last().text = kEllipsis;

  // Draw labels.
  painter->save();
  painter->setRenderHints(QPainter::Antialiasing);

  if (alignment == Qt::AlignRight) {
    // Draw right-aligned.
    int x = rect.x() + rect.width();

    for (int i = labels.size() - 1; i >= 0; --i) {
      Label label = labels.at(i);
      QSize labelSize = size(font, label);
      QRect labelRect(QPoint(x - labelSize.width(), rect.y()), labelSize);
      paint(painter, label, labelRect, selected, active);
      x -= labelSize.width() + kSpacing;
    }

    painter->restore();
    return rect.x() + rect.width() - x;
  }

  // Draw left-aligned.
  int x = rect.x();

  for (int i = 0; i < labels.size(); i++) {
    Label label = labels.at(i);
    QSize labelSize = size(font, label);
    QRect labelRect(QPoint(x, rect.y()), labelSize);
    paint(painter, label, labelRect, selected, active);
    x += labelSize.width() + kSpacing;
  }

  painter->restore();
  return x;
}

void Badge::paintEvent(QPaintEvent *event) {
  if (mLabels.isEmpty())
    return;

  QPainter painter(this);
  paint(&painter, mLabels, rect());
}

void Badge::paint(QPainter *painter, const Label &label, const QRect &rect,
                  bool selected, bool active) {
  Theme *theme = Application::theme();
  Theme::BadgeState state = Theme::BadgeState::Normal;
  if (selected && active) {
    state = Theme::BadgeState::Selected;
  } else if (label.bold) {
    state = Theme::BadgeState::Head;
  }

  if (label.type == Label::Type::Status) {
    if (label.text == '!') {
      state = Theme::BadgeState::Conflicted;
    } else if (label.text == 'M') {
      state = Theme::BadgeState::Modified;
    } else if (label.text == 'A') {
      state = Theme::BadgeState::Added;
    } else if (label.text == 'D') {
      state = Theme::BadgeState::Deleted;
    } else if (label.text == '?') {
      state = Theme::BadgeState::Untracked;
    } else if (label.text == 'R') {
      state = Theme::BadgeState::Renamed;
    }
  }

  const QColor fore = theme->badge(Theme::BadgeRole::Foreground, state);
  const QColor back = theme->badge(Theme::BadgeRole::Background, state);

  painter->setBrush(back);
  painter->setPen(Qt::NoPen);
  painter->drawRoundedRect(rect, 4, 4);

  QFont font = painter->font();
  font.setBold(label.bold);
  painter->setFont(font);

  QRect adjusted = rect;
  if (label.tag) {
    qreal x = rect.x() + (kPadding / 2) + 5;
    qreal y = rect.y() + (rect.height() / 2) + 1;
    QPainterPath path;
    path.moveTo(x, y - 5);
    path.lineTo(x - 3.5, y - 5);
    path.quadTo(x - 5, y - 5, x - 5, y - 3.5);
    path.lineTo(x - 5, y);
    path.lineTo(x - 1.5, y + 3.5);
    path.quadTo(x, y + 5, x + 1.5, y + 3.5);
    path.lineTo(x + 3.5, y + 1.5);
    path.quadTo(x + 5, y, x + 3.5, y - 1.5);
    path.closeSubpath();
    painter->fillPath(path, fore);
    painter->drawEllipse(QRectF(x - 3.5, y - 3.5, 2, 2));
    adjusted.setX(rect.x() + kIconWidth);
  }

  painter->setPen(fore);
  painter->drawText(adjusted, kLabelAlignment, label.text);
}
