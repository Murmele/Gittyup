//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <QTreeView>

class QItemDelegate;

class TreeView : public QTreeView
{
  Q_OBJECT

public:
  TreeView(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;
  bool eventFilter(QObject *obj, QEvent *event) override;
  void deselectAll();
  /*!
   * \brief countCollapsed
   * Counts the number of collapsed items. In \sa DoubleTreeWidget it is used to
   * change the state of the expand/collapse all buttons
   * \param parent Parent item to be able to use this method recursive
   * \return
   */
  int countCollapsed(QModelIndex parent = QModelIndex());
public slots:
  /*!
   * \brief expandAll
   * Expand all items
   * reimplemented to send a signal when expanded
   */
  void expandAll();
  /*!
   * \brief collapseAll
   * Collapse all items
   * reimplemented to send a signal when expanded
   */
  void collapseAll();
  /*!
   * \brief itemExpanded
   * Triggered when the expansion of an item changed
   * \param index Item with the changed expansion state
   */
  void itemExpanded(const QModelIndex& index);
  /*!
   * \brief itemCollapsed
   * Triggered when the expansion of an item changed
   * \param index Item with the changed expansion state
   */
  void itemCollapsed(const QModelIndex& index);

signals:
  void linkActivated(const QString &link);
  void fileSelected(const QModelIndex &index);
  void collapseCountChanged(int count);

private:
  /*!
   * \brief setCollapseCount
   * Used to set the collapse count and emit a signal with the new count.
   * \param value
   */
  void setCollapseCount(int value);
  /*!
   * \brief updateCollapseCount
   * update collapse count when item data is changed. In \sa DoubleTreeWidget two of these Treeviews
   * are used. One shows the staged and one the unstaged files. When a file is staged, it might appear
   * in the other TreeView. So it must be checked and the collapse count must be recalculated
   * \param topLeft
   * \param bottomRight
   * \param roles
   */
  void updateCollapseCount(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
  void updateCollapseCount(const QModelIndex &parent, int first, int last);
  void handleSelectionChange(
    const QItemSelection &selected,
	const QItemSelection &deselected);
  bool suppressDeselectionHandling{false};
  int mCollapseCount;
  bool mSupressItemExpandStateChanged{false};

  QItemDelegate *mSharedDelegate;
};

#endif // TREEVIEW_H
