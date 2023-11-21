#include "ViewDelegate.h"
#include "TreeModel.h"
#include "Badge.h"

#include <QPainter>
#include <QPainterPath>

void ViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const {
  QStyleOptionViewItem opt = option;
  drawBackground(painter, opt, index);

  // Draw badges.
  QString status = index.data(TreeModel::StatusRole).toString();
  if (!status.isEmpty()) {
    QSize size =
        Badge::size(painter->font(), Badge::Label(Badge::Label::Type::Status));
    int width = size.width();
    int height = size.height();

    auto startIter = status.cbegin(), endIter = status.cend();
    int leftAdjust = 0, rightAdjust = -3, leftWidth = 0, rightWidth = -width;
    if (mMultiColumn) {
      leftAdjust = 3;
      rightAdjust = 0;
      leftWidth = width;
      rightWidth = 0;
      std::reverse(status.begin(), status.end());
    }

    // Add extra space.
    opt.rect.adjust(leftAdjust, 0, rightAdjust, 0);

    for (int i = 0; i < status.size(); ++i) {
      int x = opt.rect.x() + opt.rect.width();
      int y = opt.rect.y() + (opt.rect.height() / 2);
      QRect rect(mMultiColumn ? opt.rect.x() : x - width, y - (height / 2),
                 width, height);
      Badge::paint(painter,
                   {Badge::Label(Badge::Label::Type::Status, status.at(i))},
                   rect, &opt);

      // Adjust rect.
      opt.rect.adjust(leftWidth + leftAdjust, 0, rightWidth + rightAdjust, 0);
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
