//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#ifndef EXTERNALTOOLSTABLEMODEL_H
#define EXTERNALTOOLSTABLEMODEL_H

#include "tools/ExternalTool.h"
#include <QAbstractTableModel>

class ExternalToolsModel : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column { Name, Command, Arguments };

  ExternalToolsModel(const QString &type, bool detected,
                     QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole);

  void refresh();
  void add(const QString &path);
  void remove(const QString &name);

private:
  QList<ExternalTool::Info> mEntries;
  QString mType;
  bool mDetected;
};

#endif
