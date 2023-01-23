//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "IndexModel.h"

IndexModel::IndexModel(Index *index, int limit, QObject *parent)
    : QAbstractItemModel(parent), mIndex(index), mLimit(limit) {
  if (mLimit < 0)
    mLimit = INT_MAX;
}

void IndexModel::setFilter(const QString &prefix) {
  mPrefix = prefix;
  mValid = false;
}

QModelIndex IndexModel::index(int row, int column,
                              const QModelIndex &parent) const {
  return createIndex(row, column, parent.isValid() ? parent.row() + 1 : 0);
}

QModelIndex IndexModel::parent(const QModelIndex &index) const {
  quintptr id = index.internalId();
  return id ? createIndex(id - 1, 0) : QModelIndex();
}

int IndexModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return map().size();

  if (parent.internalId())
    return 0;

  return qMin(map().value(map().keys().at(parent.row())).size(), mLimit);
}

int IndexModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant IndexModel::data(const QModelIndex &index, int role) const {
  switch (role) {
    case Qt::DisplayRole: {
      quintptr id = index.internalId();
      QList<Index::Field> keys = map().keys();
      if (!id)
        return Index::fieldName(keys.at(index.row()));
      return map().value(keys.at(id - 1)).at(index.row());
    }

    default:
      return QVariant();
  }
}

const QMap<Index::Field, QStringList> &IndexModel::map() const {
  if (!mValid) {
    mMap = mIndex->fieldMap(mPrefix);
    mValid = true;
  }

  return mMap;
}
