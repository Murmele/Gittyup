//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef INDEXMODEL_H
#define INDEXMODEL_H

#include "Index.h"
#include <QAbstractItemModel>
#include <QMap>

class IndexModel : public QAbstractItemModel {
  Q_OBJECT

public:
  IndexModel(Index *index, int limit = -1, QObject *parent = nullptr);

  void setFilter(const QString &prefix);

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;

private:
  const QMap<Index::Field, QStringList> &map() const;

  Index *mIndex;
  int mLimit = -1;
  QString mPrefix;

  mutable bool mValid = false;
  mutable QMap<Index::Field, QStringList> mMap;
};

#endif
