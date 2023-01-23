//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#ifndef REMOTETABLEMODEL_H
#define REMOTETABLEMODEL_H

#include "git/Repository.h"
#include <QAbstractTableModel>

class RemoteTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column { Name = 0, Url };

  RemoteTableModel(QObject *parent, const git::Repository &repo);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole);

private:
  git::Repository mRepo;
};

#endif
