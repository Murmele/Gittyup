//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SubmoduleDelegate.h"
#include "SubmoduleTableModel.h"
#include "git/Branch.h"
#include "git/Repository.h"
#include "git/Submodule.h"
#include <QComboBox>

SubmoduleDelegate::SubmoduleDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

QWidget *SubmoduleDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const {
  // Create a reference list in the upstream column.
  if (index.column() != SubmoduleTableModel::Branch)
    return QStyledItemDelegate::createEditor(parent, option, index);

  QVariant var = index.data(SubmoduleTableModel::SubmoduleRole);
  git::Repository repo = var.value<git::Submodule>().open();

  QComboBox *cb = new QComboBox(parent);
  cb->addItem(QString()); // empty name
  foreach (const git::Branch &branch, repo.branches(GIT_BRANCH_LOCAL))
    cb->addItem(branch.name());

  return cb;
}

void SubmoduleDelegate::setEditorData(QWidget *editor,
                                      const QModelIndex &index) const {
  if (QComboBox *cb = qobject_cast<QComboBox *>(editor)) {
    // Search for the matching item.
    int idx = cb->findText(index.data(Qt::DisplayRole).toString());
    if (idx >= 0)
      cb->setCurrentIndex(idx);
    return;
  }

  QStyledItemDelegate::setEditorData(editor, index);
}

void SubmoduleDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                     const QModelIndex &index) const {
  if (QComboBox *cb = qobject_cast<QComboBox *>(editor)) {
    // Set current text.
    model->setData(index, cb->currentText(), Qt::EditRole);
    return;
  }

  QStyledItemDelegate::setModelData(editor, model, index);
}
