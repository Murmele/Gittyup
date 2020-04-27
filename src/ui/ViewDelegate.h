#ifndef VIEWDELEGATE_H
#define VIEWDELEGATE_H

#include <QItemDelegate>

/*!
 * \brief The ViewDelegate class
 * Delegate used by the ColumnView and TreeView to paint the badges (M/A/?)
 */
class ViewDelegate : public QItemDelegate
{
public:
  ViewDelegate(QObject *parent = nullptr)
    : QItemDelegate(parent)
  {}

  void paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override;

  QSize sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const override;
};

#endif // VIEWDELEGATE_H
