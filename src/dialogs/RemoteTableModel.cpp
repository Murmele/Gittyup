//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#include "RemoteTableModel.h"
#include "git/Remote.h"

RemoteTableModel::RemoteTableModel(QObject *parent, const git::Repository &repo)
    : QAbstractTableModel(parent), mRepo(repo) {
  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::remoteAboutToBeAdded, this,
          &RemoteTableModel::beginResetModel);
  connect(notifier, &git::RepositoryNotifier::remoteAdded, this,
          &RemoteTableModel::endResetModel);
  connect(notifier, &git::RepositoryNotifier::remoteAboutToBeRemoved, this,
          &RemoteTableModel::beginResetModel);
  connect(notifier, &git::RepositoryNotifier::remoteRemoved, this,
          &RemoteTableModel::endResetModel);
}

int RemoteTableModel::rowCount(const QModelIndex &parent) const {
  return mRepo.remotes().count();
}

int RemoteTableModel::columnCount(const QModelIndex &parent) const { return 2; }

QVariant RemoteTableModel::data(const QModelIndex &index, int role) const {
  git::Remote remote = mRepo.remotes().at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch (index.column()) {
        case Name:
          return remote.name();
        case Url:
          return remote.url();
        default:
          return QVariant();
      }
  }

  return QVariant();
}

QVariant RemoteTableModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    return QVariant();

  switch (section) {
    case Name:
      return tr("Name");
    case Url:
      return tr("URL");
    default:
      return QVariant();
  }
}

Qt::ItemFlags RemoteTableModel::flags(const QModelIndex &index) const {
  return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool RemoteTableModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {
  if (!index.isValid() || role != Qt::EditRole || value.isNull())
    return false;

  git::Remote remote = mRepo.remotes().at(index.row());
  switch (index.column()) {
    case Name:
      remote.setName(value.toString());
      break;

    case Url:
      remote.setUrl(value.toString());
      break;
  }

  emit dataChanged(index, index);
  return true;
}
