//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "LogView.h"
#include "LogDelegate.h"
#include "LogEntry.h"
#include "LogModel.h"
#include <QAction>
#include <QApplication>
#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QHeaderView>
#include <QMimeData>
#include <QMouseEvent>

namespace {

QModelIndexList collectSelectedIndexes(QTreeView *view,
                                       const QModelIndex &parent) {
  QModelIndexList selection;
  QAbstractItemModel *model = view->model();
  for (int i = 0; i < model->rowCount(parent); ++i) {
    QModelIndex index = model->index(i, 0, parent);
    if (view->selectionModel()->isSelected(index))
      selection.append(index);
    if (view->isExpanded(index))
      selection.append(collectSelectedIndexes(view, index));
  }
  return selection;
}

} // namespace

LogView::LogView(LogEntry *root, QWidget *parent) : QTreeView(parent) {
  setMouseTracking(true);
  setHeaderHidden(true);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setContextMenuPolicy(Qt::ActionsContextMenu);

  QHeaderView *header = this->header();
  header->setStretchLastSection(false);
  header->setSectionResizeMode(QHeaderView::ResizeToContents);

  QAction *copyAction = new QAction(tr("Copy"), this);
  addAction(copyAction);

  connect(copyAction, &QAction::triggered, this, &LogView::copy);

  LogModel *model = new LogModel(root, style(), this);
  LogDelegate *delegate = new LogDelegate(this);
  connect(model, &LogModel::dataChanged, delegate,
          &LogDelegate::invalidateCache);

  setModel(model);
  setItemDelegate(delegate);

  connect(model, &QAbstractItemModel::rowsInserted,
          [this](const QModelIndex &parent, int first, int last) {
            if (!parent.isValid() && mCollapse)
              collapse(this->model()->index(last - 1, 0, parent));
            scrollTo(this->model()->index(last, 0, parent));
          });
}

void LogView::setCollapseEnabled(bool collapse) { mCollapse = collapse; }

void LogView::setEntryExpanded(LogEntry *entry, bool expanded) {
  setExpanded(static_cast<LogModel *>(model())->index(entry), expanded);
}

void LogView::copy() {
  QString plainText;
  QString richText;
  QModelIndexList indexes = collectSelectedIndexes(this, QModelIndex());
  foreach (const QModelIndex &index, indexes) {
    QString prefix;

    // Indent child indices
    QModelIndex currentParent = index.parent();
    while (currentParent.isValid()) {
      prefix += QString(4, ' ');
      currentParent = currentParent.parent();
    }

    // Add expansion indicator
    if (this->model()->hasChildren(index))
      prefix += isExpanded(index) ? "[-]" : "[+]";

    QString text = index.data().toString();
    plainText +=
        QString("%1 %2\n").arg(prefix, text.remove(QRegExp("<[^>]*>")));
    richText += QString("%1 %2<br>").arg(prefix, text);
  }

  richText = QString("<pre>%1</pre>").arg(richText);

  QMimeData *mimeDate = new QMimeData;
  mimeDate->setText(plainText);
  mimeDate->setHtml(richText);
  QApplication::clipboard()->setMimeData(mimeDate);
}

QSize LogView::minimumSizeHint() const {
  return QSize(QTreeView::minimumSizeHint().width(), 0);
}

void LogView::mouseMoveEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);

  QString link = linkAt(index, pos);
  setCursor(link.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);

  if (!mLink.isEmpty() || mCancel.isValid())
    return;

  QTreeView::mouseMoveEvent(event);
}

void LogView::mousePressEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);

  mLink = linkAt(index, pos);

  mCancel = QModelIndex();
  if (isDecoration(index, pos) &&
      index.data(LogModel::EntryRole).value<LogEntry *>()->progress() >= 0)
    mCancel = index;

  if (!mLink.isEmpty() || mCancel.isValid())
    return;

  QTreeView::mousePressEvent(event);
}

void LogView::mouseReleaseEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);

  if (!mLink.isEmpty() && mLink == linkAt(index, pos)) {
    if (mLink == "expand") {
      setExpanded(index, !isExpanded(index));
    } else {
      emit linkActivated(mLink);
    }
  }

  if (mCancel == index && isDecoration(index, pos))
    emit operationCanceled(index);

  if (!mLink.isEmpty() || mCancel.isValid()) {
    mLink = QString();
    mCancel = QModelIndex();
    return;
  }

  QTreeView::mouseReleaseEvent(event);
}

QString LogView::linkAt(const QModelIndex &index, const QPoint &pos) {
  if (!index.isValid())
    return QString();

  LogDelegate *delegate = static_cast<LogDelegate *>(itemDelegate());
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);
  QPoint docPos = pos - delegate->documentPosition(options, index);
  return delegate->document(index)->documentLayout()->anchorAt(docPos);
}

bool LogView::isDecoration(const QModelIndex &index, const QPoint &pos) {
  if (!index.isValid())
    return false;

  LogDelegate *delegate = static_cast<LogDelegate *>(itemDelegate());
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);
  return delegate->decorationRect(options, index).contains(pos);
}
