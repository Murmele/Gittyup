//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "TreeView.h"
#include "ColumnView.h"
#include "ViewDelegate.h"
#include "TreeModel.h"
#include <QFormLayout>
#include <QItemDelegate>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#define ICON_SIZE 48
#define SCROLL_BAR_WIDTH 18
#else
#define ICON_SIZE 64
#define SCROLL_BAR_WIDTH 0
#endif

namespace {

const QString kNameFmt = "<p style='font-size: large'>%1</p>";
const QString kLabelFmt = "<p style='color: gray; font-weight: bold'>%1</p>";

} // anon. namespace

TreeView::TreeView(QWidget *parent)
  : QTreeView(parent), mSharedDelegate(new ViewDelegate(this))
{
}

void TreeView::setModel(QAbstractItemModel *model)
{
  QTreeView::setModel(model);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged,
		  this, &TreeView::handleSelectionChange);
  connect(model, &QAbstractItemModel::dataChanged, this, QOverload<const QModelIndex &, const QModelIndex&, const QVector<int>&>::of(&TreeView::updateCollapseCount));
  connect(this, &QTreeView::collapsed, this, &TreeView::itemCollapsed);
  connect(this, &QTreeView::expanded, this, &TreeView::itemExpanded);
  connect(model, &QAbstractItemModel::rowsInserted, this, QOverload<const QModelIndex &, int, int>::of(&TreeView::updateCollapseCount));
}

bool TreeView::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress) {
	QWidget *TreeViewport = static_cast<QWidget *>(obj);
    QPoint globalPos = static_cast<QMouseEvent *>(event)->globalPos();
    QModelIndex index = indexAt(viewport()->mapFromGlobal(globalPos));
	if (!TreeViewport->hasFocus() && index.row() < 0) {
	  TreeViewport->setFocus();
      selectionModel()->clearSelection();
    }
  }

  return false;
}

void TreeView::handleSelectionChange(
  const QItemSelection &selected,
  const QItemSelection &deselected)
{
  // FIXME: The argument sent by Qt doesn't contain the whole selection.
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  if (indexes.length() > 0) {
	  QModelIndex index = (indexes.size() == 1) ? indexes.first() : QModelIndex();
	  emit fileSelected(index);
  }

  // ignore deselection handling, because when selecting an item in the second
  // TreeView (staged/unstaged files), the root should not be set selected. Anything
  // should be selected in this View
  if (suppressDeselectionHandling)
	  return;

  // Handle deselection.
  if (indexes.isEmpty() && !deselected.indexes().isEmpty()) {
    QModelIndex parent = deselected.indexes().first().parent();
    setCurrentIndex(parent);
    if (!parent.isValid())
      setRootIndex(QModelIndex());
  }
}

void TreeView::setCollapseCount(int value)
{
    assert(value >= 0);
    mCollapseCount = value;
    emit collapseCountChanged(mCollapseCount);
}

void TreeView::updateCollapseCount(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    assert(topLeft == bottomRight); // makes no sense that they are different here. See also the TreeModel

    if (roles[0] != Qt::CheckStateRole)
        return;

    setCollapseCount(countCollapsed());

}

void TreeView::updateCollapseCount(const QModelIndex &parent, int first, int last)
{
    setCollapseCount(countCollapsed());
}

int TreeView::countCollapsed(QModelIndex parent)
{
    QAbstractItemModel* model = this->model();

    int count = 0;
    for (int i=0; i < model->rowCount(parent); i++) {
        QModelIndex idx = model->index(i, 0, parent);
        if (model->rowCount(idx) && !this->isExpanded(idx))
            count++;
        count += countCollapsed(idx);
    }
    return count;
}

void TreeView::expandAll()
{
    mSupressItemExpandStateChanged = true;
    QTreeView::expandAll();
    mSupressItemExpandStateChanged = false;
    setCollapseCount(0);
}

void TreeView::collapseAll()
{
    mSupressItemExpandStateChanged = true;
    QTreeView::collapseAll();
    mSupressItemExpandStateChanged = false;
    setCollapseCount(countCollapsed());
}

void TreeView::itemExpanded(const QModelIndex& index)
{
    if (mSupressItemExpandStateChanged)
        return;

    setCollapseCount(mCollapseCount - 1);
}

void TreeView::itemCollapsed(const QModelIndex& index)
{
    if (mSupressItemExpandStateChanged)
        return;

    setCollapseCount(mCollapseCount + 1);
}



void TreeView::deselectAll() {
	suppressDeselectionHandling = true;
	selectionModel()->clearSelection();
	suppressDeselectionHandling = false;
}
