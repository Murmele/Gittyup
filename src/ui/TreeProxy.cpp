//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TreeProxy.h"
#include "DiffTreeModel.h"
#include "conf/Settings.h"
#include "git/Blob.h"
#include "git/Diff.h"
#include "git/RevWalk.h"
#include "git/Submodule.h"
#include <QStringBuilder>
#include <QUrl>

namespace {

const QString kLinkFmt = "<a href='%1'>%2</a>";

} // namespace

TreeProxy::TreeProxy(bool staged, QObject *parent)
    : QSortFilterProxyModel(parent), mStaged(staged) {}

TreeProxy::~TreeProxy() {}

bool TreeProxy::setData(const QModelIndex &index, const QVariant &value,
                        int role, bool ignoreIndexChanges) {
  QModelIndex sourceIndex = mapToSource(index);
  if (index.isValid() && !sourceIndex.isValid())
    return false;

  return static_cast<DiffTreeModel *>(sourceModel())
      ->setData(sourceIndex, value, role, ignoreIndexChanges);
}

bool TreeProxy::filterAcceptsRow(int source_row,
                                 const QModelIndex &source_parent) const {
  QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
  if (!index.isValid())
    return false;

  if (!mFilter)
    return true;

  // This is anymore needed, because only the diff tree is checked and so every
  // file is modified or was added
  //    QString status = sourceModel()->data(index,
  //    DiffTreeModel::StatusRole).toString();
  //	QRegExp regexp(".*[AM?].*");
  //	if (!status.contains(regexp))
  //		return false; // if the file/folder was not modified/added ...
  // don't show it in the tree view

  Qt::CheckState state = static_cast<Qt::CheckState>(
      sourceModel()->data(index, Qt::CheckStateRole).toInt());
  if (mStaged && state == Qt::CheckState::Unchecked)
    return false;
  else if (!mStaged && state == Qt::CheckState::Checked)
    return false;

  return true;
}
