//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SubmoduleTableModel.h"
#include "git/Repository.h"
#include "git/Submodule.h"
#include <QMessageBox>
#include <QPushButton>

SubmoduleTableModel::SubmoduleTableModel(const git::Repository &repo,
                                         QObject *parent)
    : QAbstractTableModel(parent), mSubmodules(repo.submodules()), mRepo(repo) {
}

int SubmoduleTableModel::rowCount(const QModelIndex &parent) const {
  return mSubmodules.count();
}

int SubmoduleTableModel::columnCount(const QModelIndex &parent) const {
  return 4;
}

QVariant SubmoduleTableModel::data(const QModelIndex &index, int role) const {
  git::Submodule submodule = mSubmodules.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch (index.column()) {
        case Name:
          return submodule.name();
        case Url:
          return submodule.url();
        case Branch:
          return submodule.branch();
        case Init:
          return QVariant();
      }

    case Qt::CheckStateRole:
      if (index.column() == Init)
        return submodule.isInitialized() ? Qt::Checked : Qt::Unchecked;
      return QVariant();

    case SubmoduleRole:
      return QVariant::fromValue(submodule);
  }

  return QVariant();
}

QVariant SubmoduleTableModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    return QVariant();

  switch (section) {
    case Name:
      return tr("Name");
    case Url:
      return tr("URL");
    case Branch:
      return tr("Branch");
    case Init:
      return tr("Initialized");
  }

  return QVariant();
}

Qt::ItemFlags SubmoduleTableModel::flags(const QModelIndex &index) const {
  switch (index.column()) {
    case Name:
      return QAbstractTableModel::flags(index) | Qt::NoItemFlags;
    case Init:
      return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;
    default:
      return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
  }
}

bool SubmoduleTableModel::setData(const QModelIndex &index,
                                  const QVariant &value, int role) {
  if (!index.isValid())
    return false;

  git::Submodule submodule = mSubmodules.at(index.row());
  switch (index.column()) {
    case Name:
      return false;

    case Url:
      submodule.setUrl(value.toString());
      break;

    case Branch: {
      QString name = value.toString();
      if (!name.isEmpty() || !submodule.branch().isEmpty())
        submodule.setBranch(value.toString());
      break;
    }

    case Init: {
      if (value == Qt::Checked) {
        submodule.initialize();
        break;
      }

      QDir dir(mRepo.workdir().filePath(submodule.path()));
      if (dir.isEmpty()) {
        submodule.deinitialize();
        break;
      }

      QString text =
          tr("Deinitializing '%1' will remove its working directory. Are "
             "you sure you want to deinitialize?")
              .arg(submodule.name());
      QMessageBox *mb = new QMessageBox(
          QMessageBox::Warning, tr("Deinitialize Submodule?"), text,
          QMessageBox::Cancel, qobject_cast<QWidget *>(parent()));

      if (GIT_SUBMODULE_STATUS_IS_WD_DIRTY(
              mRepo.submoduleStatus(submodule.name())))
        mb->setInformativeText(
            tr("The submodule working directory contains uncommitted "
               "changes that will be lost if you continue."));

      QPushButton *deinit =
          mb->addButton(tr("Deinitialize"), QMessageBox::AcceptRole);
      mb->setDefaultButton(deinit);
      connect(deinit, &QPushButton::clicked,
              [submodule] { submodule.deinitialize(); });

      mb->open();
      break;
    }
  }

  emit dataChanged(index, index);
  return true;
}
