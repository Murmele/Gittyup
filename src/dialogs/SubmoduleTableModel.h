//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SUBMODULETABLEMODEL_H
#define SUBMODULETABLEMODEL_H

#include "git/Submodule.h"
#include <QAbstractTableModel>

namespace git {
class Repository;
}

class SubmoduleTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Role { SubmoduleRole = Qt::UserRole };

  enum Column { Name, Url, Branch, Init };

  SubmoduleTableModel(const git::Repository &repo, QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole);

private:
  QList<git::Submodule> mSubmodules;
  git::Repository mRepo;
};

#endif
