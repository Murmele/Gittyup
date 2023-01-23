//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#include "BranchDelegate.h"
#include "BranchTableModel.h"
#include "ui/ReferenceList.h"

BranchDelegate::BranchDelegate(const git::Repository &repo, QObject *parent)
    : QStyledItemDelegate(parent), mRepo(repo) {}

QWidget *BranchDelegate::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const {
  // Create a reference list in the upstream column.
  if (index.column() != BranchTableModel::Upstream)
    return QStyledItemDelegate::createEditor(parent, option, index);

  auto kinds = ReferenceView::InvalidRef | ReferenceView::RemoteBranches;
  ReferenceList *refs = new ReferenceList(mRepo, kinds, parent);
  return refs;
}

void BranchDelegate::setEditorData(QWidget *editor,
                                   const QModelIndex &index) const {
  ReferenceList *refs = qobject_cast<ReferenceList *>(editor);
  if (!refs) {
    QStyledItemDelegate::setEditorData(editor, index);
    return;
  }

  // Search for the matching item.
  int idx = refs->findText(index.data(Qt::DisplayRole).toString());
  if (idx >= 0)
    refs->setCurrentIndex(idx);
}

void BranchDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                  const QModelIndex &index) const {
  ReferenceList *refs = qobject_cast<ReferenceList *>(editor);
  if (!refs) {
    QStyledItemDelegate::setModelData(editor, model, index);
    return;
  }

  // Look up the reference object at the current index.
  model->setData(index, refs->currentData(Qt::UserRole), Qt::EditRole);
}
