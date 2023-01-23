//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//          Copyright (c) 2022, Gittyup Contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef REFERENCEMODEL_H
#define REFERENCEMODEL_H

#include <QAbstractItemModel>
#include "ReferenceView.h"
#include "git/Reference.h"

class ReferenceModel : public QAbstractItemModel {
  Q_OBJECT

public:
  enum ReferenceType {
    COMBOBOX_HEADER = 0,
    Branches = 1,
    Remotes = 2,
    Tags = 3
  };

  struct ReferenceList {
    QString name;
    QList<git::Reference> refs;
    ReferenceType type;
  };

  ReferenceModel(const git::Repository &repo, ReferenceView::Kinds kinds,
                 QObject *parent = nullptr);
  int referenceTypeToIndex(ReferenceType t) const;
  int indexToReferenceType(int index) const;
  void setCommit(const git::Commit &commit);
  void update();
  QModelIndex firstRemote();
  QModelIndex firstBranch();
  QModelIndex firstTag();
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

private:
  git::Repository mRepo;
  ReferenceView::Kinds mKinds;
  QList<ReferenceList> mRefs;
  git::Commit mCommit;
};

#endif // REFERENCEMODEL_H
