//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TreeProxy.h"

TreeProxy::TreeProxy(bool staged, QObject *parent)
  : QSortFilterProxyModel(parent), mStaged(staged)
{}

TreeProxy::~TreeProxy()
{}

bool TreeProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
  QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
  if (!index.isValid())
    return false;

  Qt::CheckState state = static_cast<Qt::CheckState>(sourceModel()->data(index, Qt::CheckStateRole).toInt());
  if (mStaged && state == Qt::CheckState::Unchecked)
    return false;
  else if (!mStaged && state == Qt::CheckState::Checked)
    return false;

  return true;
}
