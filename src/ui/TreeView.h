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
#include <memory>
#include "ViewDelegate.h"

class QItemDelegate;
class DiffTreeModel;

class TreeView : public QTreeView {
  Q_OBJECT

public:
  TreeView(QWidget *parent = nullptr, const QString &name = QString());

  void discard(const QModelIndex &index, const bool force = false);
  void discard(DiffTreeModel *model, const QModelIndex &index);
  void setModel(QAbstractItemModel *model) override;
  bool eventFilter(QObject *obj, QEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void deselectAll();
  /*!
   * Get the rectangle occupied by the item's checkboy.
   * Used in the UI tests
   */
  QRect checkRect(const QModelIndex &index);
  /*!
   * \brief countCollapsed
   * Counts the number of collapsed items. In \sa DoubleTreeWidget it is used to
   * change the state of the expand/collapse all buttons
   * \param parent Parent item to be able to use this method recursive
   * \return
   */
  int countCollapsed(QModelIndex parent = QModelIndex(), bool recursive = true);
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
  void itemExpanded(const QModelIndex &index);
  /*!
   * \brief itemCollapsed
   * Triggered when the expansion of an item changed
   * \param index Item with the changed expansion state
   */
  void itemCollapsed(const QModelIndex &index);

signals:
  void linkActivated(const QString &link);
  void filesSelected(const QModelIndexList &indexes);
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
   * update collapse count when item data is changed. In \sa DoubleTreeWidget
   * two of these Treeviews are used. One shows the staged and one the unstaged
   * files. When a file is staged, it might appear in the other TreeView. So it
   * must be checked and the collapse count must be recalculated \param topLeft
   * \param bottomRight
   * \param roles
   */
  void updateCollapseCount(const QModelIndex &topLeft,
                           const QModelIndex &bottomRight,
                           const QVector<int> &roles = QVector<int>());
  void updateCollapseCount(const QModelIndex &parent, int first, int last);
  void handleSelectionChange(const QItemSelection &selected,
                             const QItemSelection &deselected);
  bool suppressDeselectionHandling{false};
  int mCollapseCount; // Counts the number of collapsed folders.
  bool mSupressItemExpandStateChanged{false};
  void updateView();

  QString mName;
  std::unique_ptr<ViewDelegate> mFileListDelegatePtr;
  std::unique_ptr<ViewDelegate> mFileTreeDelegatePtr;
  int mDelegateCol{false};
};

#endif // TREEVIEW_H
