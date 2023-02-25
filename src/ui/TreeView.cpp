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
#include "Debug.h"
#include <QFormLayout>
#include <QItemDelegate>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include "TreeProxy.h"
#include <QMenu>
#include "DiffTreeModel.h"
#include "RepoView.h"
#include <QMessageBox>
#include <QPushButton>

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

} // namespace

TreeView::TreeView(QWidget *parent, const QString &name)
    : QTreeView(parent), mSharedDelegate(new ViewDelegate(this)), mName(name) {
  setObjectName(name);
}

void TreeView::setModel(QAbstractItemModel *model) {
  QTreeView::setModel(model);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &TreeView::handleSelectionChange);
  connect(model, &QAbstractItemModel::dataChanged, this,
          QOverload<const QModelIndex &, const QModelIndex &,
                    const QVector<int> &>::of(&TreeView::updateCollapseCount));
  connect(this, &QTreeView::collapsed, this, &TreeView::itemCollapsed);
  connect(this, &QTreeView::expanded, this, &TreeView::itemExpanded);
  connect(model, &QAbstractItemModel::rowsInserted, this,
          QOverload<const QModelIndex &, int, int>::of(
              &TreeView::updateCollapseCount));
}

void TreeView::discard(const QModelIndex &index, const bool force) {
  auto p = qobject_cast<QSortFilterProxyModel *>(this->model());
  assert(p);
  auto m = qobject_cast<DiffTreeModel *>(p->sourceModel());
  assert(m);
  auto sIndex = p->mapToSource(index);
  int patchIndex = sIndex.data(DiffTreeModel::Role::PatchIndexRole).toInt();
  QString name = sIndex.data(Qt::DisplayRole).toString();

  if (force)
    this->discard(m, sIndex);
  else {
    QString arg = patchIndex < 0 ? tr("Directory") : tr("File");
    QString title = tr("Remove or discard %1?").arg(name);
    QString text =
        tr("Are you sure you want to remove or discard all changes in '%1'?")
            .arg(name);
    QMessageBox *dialog = new QMessageBox(
        QMessageBox::Warning, title, text.arg(name), QMessageBox::Cancel, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInformativeText(tr("This action cannot be undone."));

    QPushButton *discard =
        dialog->addButton(tr("Discard"), QMessageBox::AcceptRole);
    connect(discard, &QPushButton::clicked,
            [this, m, sIndex] { this->discard(m, sIndex); });
    dialog->exec();
  }
}

void TreeView::discard(DiffTreeModel *model, const QModelIndex &index) {
  RepoView *view = RepoView::parentView(this);
  assert(view);
  if (!model->discard(index)) {
    QString patchName = index.data(Qt::DisplayRole).toString();
    LogEntry *parent = view->addLogEntry(patchName, tr("Discard"));
    view->error(parent, tr("discard"), patchName);
  }
  // FIXME: Work dir changed?
  view->refresh();
}

bool TreeView::eventFilter(QObject *obj, QEvent *event) {
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

/*!
 * \brief TreeView::keyPressEvent
 * Implemented, because for some reason a segfault in the binaries (probably qt)
 * happens: https://github.com/Murmele/Gittyup/issues/109 Can be removed in a
 * later version if it can be proved that the problem is solved.
 */
void TreeView::keyPressEvent(QKeyEvent *event) {
  auto index = currentIndex();
  if (index.isValid() && event->key() == Qt::Key_Space) {
    auto checkState =
        static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
    if (checkState == Qt::Checked)
      checkState = Qt::Unchecked;
    else
      checkState = Qt::Checked;
    model()->setData(index, checkState, Qt::CheckStateRole);

    // Select next item.
    QKeyEvent *down =
        new QKeyEvent(event->type(), Qt::Key_Down, event->modifiers());
    QTreeView::keyPressEvent(down);
  } else
    QTreeView::keyPressEvent(event);
}

void TreeView::handleSelectionChange(const QItemSelection &selected,
                                     const QItemSelection &deselected) {
  Q_UNUSED(selected)

  // FIXME: The argument sent by Qt doesn't contain the whole selection.
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  if (indexes.count() > 0)
    emit filesSelected(indexes);

  // ignore deselection handling, because when selecting an item in the second
  // TreeView (staged/unstaged files), the root should not be set selected.
  // Anything should be selected in this View
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

void TreeView::setCollapseCount(int value) {
  Debug("Name: " << mName << "Current Collapsed: " << mCollapseCount
                 << "; New Collapsed: " << value);
  assert(value >= 0);
  mCollapseCount = value;
  emit collapseCountChanged(mCollapseCount);
}

void TreeView::updateCollapseCount(const QModelIndex &topLeft,
                                   const QModelIndex &bottomRight,
                                   const QVector<int> &roles) {
  assert(topLeft == bottomRight); // makes no sense that they are different
                                  // here. See also the TreeModel

  if (roles[0] != Qt::CheckStateRole)
    return;

  setCollapseCount(countCollapsed());
}

void TreeView::updateCollapseCount(const QModelIndex &parent, int first,
                                   int last) {
  setCollapseCount(countCollapsed());
}

/*!
 * \brief TreeView::countCollapsed
 * Takes long for many items
 * \param parent
 * \return
 */
int TreeView::countCollapsed(QModelIndex parent, bool recursive) {
  QAbstractItemModel *model = this->model();

  int count = 0;
  for (int i = 0; i < model->rowCount(parent); i++) {
    QModelIndex idx = model->index(i, 0, parent);
    auto data = idx.data();
    if (model->hasChildren(idx) && !this->isExpanded(idx)) {
      // It is a folder but is not expanded
      count++;
    } else if (this->isExpanded(idx) && recursive) {
      // Count only if the item is expanded and if recursive is on
      count += countCollapsed(idx);
    }
    Debug("Name: " << mName << " Row: " << i << ": " << data.toString()
                   << "; Count: " << count);
  }

  Debug("Name: " << mName << ", countCollapsed():" << count);
  return count;
}

void TreeView::expandAll() {
  mSupressItemExpandStateChanged = true;
  QTreeView::expandAll();
  mSupressItemExpandStateChanged = false;
  setCollapseCount(0);
}

void TreeView::collapseAll() {
  mSupressItemExpandStateChanged = true;
  QTreeView::collapseAll();
  mSupressItemExpandStateChanged = false;

  // Count only folders not files, because they are not able to expand/collapse
  // Everything is collapsed so only the top items must be checked
  int count = 0;
  for (int i = 0; i < model()->rowCount(); i++) {
    auto child = model()->index(i, 0);
    if (model()->hasChildren(child))
      count++;
  }

  setCollapseCount(count);
}

void TreeView::itemExpanded(const QModelIndex &index) {
  if (mSupressItemExpandStateChanged)
    return;
  Debug("Expanded: Name: " << mName
                           << ", Index data: " << index.data().toString());
  setCollapseCount(mCollapseCount - 1 + countCollapsed(index, false));
}

void TreeView::itemCollapsed(const QModelIndex &index) {
  if (mSupressItemExpandStateChanged)
    return;

  Debug("Collapsed: Name: " << mName
                            << ", Index data: " << index.data().toString());
  // one item was collapsed, but all collapsed items below this item must be
  // subtracted
  setCollapseCount(mCollapseCount + 1 - countCollapsed(index, false));
}

void TreeView::deselectAll() {
  suppressDeselectionHandling = true;
  selectionModel()->clearSelection();
  suppressDeselectionHandling = false;
}

QRect TreeView::checkRect(const QModelIndex &index) {
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);
  options.features |= QStyleOptionViewItem::HasCheckIndicator;

  QStyle::SubElement se = QStyle::SE_ItemViewItemCheckIndicator;
  return style()->subElementRect(se, &options, this);
}
