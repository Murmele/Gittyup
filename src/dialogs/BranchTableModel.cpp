//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#include "BranchTableModel.h"
#include "git/Branch.h"
#include <QWidget>

BranchTableModel::BranchTableModel(const git::Repository &repo, QObject *parent)
    : QAbstractTableModel(parent), mRepo(repo) {
  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::referenceAboutToBeAdded, this,
          &BranchTableModel::beginResetModel);
  connect(notifier, &git::RepositoryNotifier::referenceAdded, this,
          &BranchTableModel::endResetModel);
  connect(notifier, &git::RepositoryNotifier::referenceAboutToBeRemoved, this,
          &BranchTableModel::beginResetModel);
  connect(notifier, &git::RepositoryNotifier::referenceRemoved, this,
          &BranchTableModel::endResetModel);
}

int BranchTableModel::rowCount(const QModelIndex &parent) const {
  return branches().count();
}

int BranchTableModel::columnCount(const QModelIndex &parent) const { return 3; }

QVariant BranchTableModel::data(const QModelIndex &index, int role) const {
  git::Branch branch = branches().at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch (index.column()) {
        case Name:
          return branch.name();

        case Upstream: {
          git::Branch upstream = branch.upstream();
          return upstream.isValid() ? upstream.name() : QVariant();
        }

        case Rebase:
          return QVariant();
      }

    case Qt::FontRole: {
      QFont font = static_cast<QWidget *>(QObject::parent())->font();
      font.setBold(index.column() == Name && branch.isHead());
      return font;
    }

    case Qt::CheckStateRole:
      if (index.column() != Rebase)
        return QVariant();
      return branch.isRebase() ? Qt::Checked : Qt::Unchecked;

    case BranchRole:
      return QVariant::fromValue(branch);
  }

  return QVariant();
}

QVariant BranchTableModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    return QVariant();

  switch (section) {
    case Name:
      return tr("Name");
    case Upstream:
      return tr("Upstream");
    case Rebase:
      return tr("Rebase");
  }

  return QVariant();
}

Qt::ItemFlags BranchTableModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags flags = QAbstractTableModel::flags(index);

  switch (index.column()) {
    case Name: {
      git::Branch branch = branches().at(index.row());
      if (!branch.isHead())
        flags |= Qt::ItemIsEditable;
      break;
    }

    case Upstream:
      flags |= Qt::ItemIsEditable;
      break;

    case Rebase:
      flags |= Qt::ItemIsUserCheckable;
      break;
  }

  return flags;
}

bool BranchTableModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {
  if (!index.isValid())
    return false;

  git::Branch branch = branches().at(index.row());
  switch (index.column()) {
    case Name:
      branch.rename(value.toString());
      break;

    case Upstream:
      branch.setUpstream(value.value<git::Reference>());
      break;

    case Rebase:
      branch.setRebase(value == Qt::Checked);
      break;
  }

  emit dataChanged(index, index);
  return true;
}

QList<git::Branch> BranchTableModel::branches() const {
  return mRepo.branches(GIT_BRANCH_LOCAL);
}
