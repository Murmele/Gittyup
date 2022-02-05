//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#include "FileContextMenu.h"
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
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &TreeView::customContextMenuRequested, this,
          &TreeView::onCustomContextMenu);
  //  connect(model, &QAbstractItemModel::modelReset, [this]{
  //      setCollapseCount(countCollapsed());});
}

void TreeView::discard(const QModelIndex &index) {
  auto p = qobject_cast<QSortFilterProxyModel *>(this->model());
  assert(p);
  auto m = qobject_cast<DiffTreeModel *>(p->sourceModel());
  assert(m);
  auto sIndex = p->mapToSource(index);
  int patchIndex = sIndex.data(DiffTreeModel::Role::PatchIndexRole).toInt();
  QString name = sIndex.data(Qt::DisplayRole).toString();

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
    connect(discard, &QPushButton::clicked, [this, m, sIndex] {
        RepoView *view = RepoView::parentView(this);
        assert(view);
        if (!m->discard(sIndex)) {
            QString patchName = sIndex.data(Qt::DisplayRole).toString();
            LogEntry *parent = view->addLogEntry(patchName, tr("Discard"));
            view->error(parent, tr("discard"), patchName);
        }
        // FIXME: Work dir changed?
        view->refresh();
    });
    dialog->exec();
}

void TreeView::onCustomContextMenu(const QPointF& point)
{
  QStringList files;
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  foreach (const QModelIndex &index, indexes)
    files.append(index.data(Qt::EditRole).toString());

  if (files.isEmpty())
    return;

  RepoView *view = RepoView::parentView(this);
  FileContextMenu menu(view, files, view->repo().index());
  menu.exec(viewport()->mapToGlobal(point.toPoint()));
}

void TreeView::onCustomContextMenu(const QPointF &point) {
  auto proxy = qobject_cast<TreeProxy *>(model());
  if (!proxy)
    return;

  QPoint p(qRound(point.x()), qRound(point.y()));
  QModelIndex index = indexAt(p);
  if (!index.isValid())
    return;

  QVariant checkState = index.data(Qt::CheckStateRole);

  if (checkState.isNull()) {
    // at the moment there is no case that a context menu
    // is shown when a commit, instead of the
    // current changes is selected
    return;
  }

  QMenu contextMenu;
  QAction a;
  //    if (proxy->staged()) {
  //        a.setText(tr("Unstage selected"));

  //    } else {
  //        a.setText(tr("Stage selected"));
  //    }
  //    contextMenu.addAction(&a);

  // means that the current head is selected where
  // changes are visible. If not valid it means a commit
  // is selected and then it should not be possible to
  // discard
  auto discardAction = QAction(tr("Discard selected"));
  contextMenu.addAction(&discardAction);
  connect(&discardAction, &QAction::triggered,
          [this, index]() { this->discard(index); });

  contextMenu.exec(viewport()->mapToGlobal(p));
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
  qDebug() << "Name: " << mName << "Current Collapsed: " << mCollapseCount
           << "; New Collapsed: " << value;
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
      count++;
    } else if (this->isExpanded(idx) && recursive) {
      // Count only if the item is expanded and if recursive is on
      count += countCollapsed(idx);
    }
    qDebug() << "Name: " << mName << " Row: " << i << ": " << data.toString()
             << "; Count: " << count;
  }

  qDebug() << "Name: " << mName << ", countCollapsed():" << count;
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
  qDebug() << "Name: " << mName << ", Index data: " << index.data().toString();
  setCollapseCount(mCollapseCount + countCollapsed(index, false));
}

void TreeView::itemCollapsed(const QModelIndex &index) {
  if (mSupressItemExpandStateChanged)
    return;

  qDebug() << "Name: " << mName << ", Index data: " << index.data().toString();
  setCollapseCount(mCollapseCount - countCollapsed(index, false));
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
