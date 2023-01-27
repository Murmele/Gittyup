#include "ViewDelegate.h"
#include "TreeModel.h"
#include "Badge.h"

#include <QPainter>
#include <QPainterPath>

void ViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const {
  QStyleOptionViewItem opt = option;
  drawBackground(painter, opt, index);

  // Draw >.
  if (mDrawArrow && index.model()->hasChildren(index)) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QColor color = opt.palette.color(QPalette::Active, QPalette::BrightText);
    if (opt.state & QStyle::State_Selected)
      color =
          !opt.showDecorationSelected
              ? opt.palette.color(QPalette::Active, QPalette::WindowText)
              : opt.palette.color(QPalette::Active, QPalette::HighlightedText);

    painter->setPen(color);
    painter->setBrush(color);

    int x = opt.rect.x() + opt.rect.width() - 3;
    int y = opt.rect.y() + (opt.rect.height() / 2);

    QPainterPath path;
    path.moveTo(x, y);
    path.lineTo(x - 5, y - 3);
    path.lineTo(x - 5, y + 3);
    path.closeSubpath();
    painter->drawPath(path);

    painter->restore();

    // Adjust rect to exclude the arrow.
    opt.rect.adjust(0, 0, -11, 0);
  }

  // Draw badges.
  QString status = index.data(TreeModel::StatusRole).toString();
  if (!status.isEmpty()) {
    QSize size =
        Badge::size(painter->font(), Badge::Label(Badge::Label::Type::Status));
    int width = size.width();
    int height = size.height();

    // Add extra space.
    opt.rect.adjust(0, 0, -3, 0);

    for (int i = 0; i < status.count(); ++i) {
      int x = opt.rect.x() + opt.rect.width();
      int y = opt.rect.y() + (opt.rect.height() / 2);
      QRect rect(x - width, y - (height / 2), width, height);
      Badge::paint(painter,
                   {Badge::Label(Badge::Label::Type::Status, status.at(i))},
                   rect, &opt);

      // Adjust rect.
      opt.rect.adjust(0, 0, -width - 3, 0);
    }
  }

  QItemDelegate::paint(painter, opt, index);
}

QSize ViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
  // Increase spacing.
  QSize size = QItemDelegate::sizeHint(option, index);
  size.setHeight(
      Badge::size(option.font, Badge::Label(Badge::Label::Type::Status))
          .height() +
      4);
  return size;
}
