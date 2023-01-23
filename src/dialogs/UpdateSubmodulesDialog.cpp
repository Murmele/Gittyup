//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "UpdateSubmodulesDialog.h"
#include "git/Repository.h"
#include "git/Submodule.h"
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

namespace {

class Model : public QAbstractTableModel {
public:
  Model(const git::Repository &repo, QObject *parent = nullptr)
      : QAbstractTableModel(parent) {
    foreach (const git::Submodule &submodule, repo.submodules())
      mSubmodules.append({true, submodule});
  }

  QList<git::Submodule> enabledSubmodules() const {
    QList<git::Submodule> submodules;
    foreach (const Entry &entry, mSubmodules) {
      if (entry.enabled)
        submodules.append(entry.submodule);
    }

    return submodules;
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const override {
    return 2;
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    return mSubmodules.size();
  }

  Qt::ItemFlags flags(const QModelIndex &index) const override {
    return QAbstractTableModel::flags(index) |
           ((index.column() == 0) ? Qt::ItemIsUserCheckable : Qt::NoItemFlags);
  }

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    const Entry &entry = mSubmodules.at(index.row());
    switch (role) {
      case Qt::CheckStateRole:
        if (index.column() == 0)
          return entry.enabled ? Qt::Checked : Qt::Unchecked;
        break;

      case Qt::DisplayRole:
        if (index.column() == 1)
          return entry.submodule.name();
        break;
    }

    return QVariant();
  }

  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override {
    if (index.column() != 0)
      return false;

    mSubmodules[index.row()].enabled = value.toBool();
    emit dataChanged(index, index);
    return true;
  }

private:
  struct Entry {
    bool enabled;
    git::Submodule submodule;
  };

  QList<Entry> mSubmodules;
};

} // namespace

UpdateSubmodulesDialog::UpdateSubmodulesDialog(const git::Repository &repo,
                                               QWidget *parent)
    : QDialog(parent) {
  mTable = new QTableView(this);
  mTable->setShowGrid(false);
  mTable->setSelectionMode(QAbstractItemView::NoSelection);

  Model *model = new Model(repo, this);
  mTable->setModel(model);

  mTable->verticalHeader()->hide();
  mTable->horizontalHeader()->hide();
  mTable->horizontalHeader()->setStretchLastSection(false);
  mTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);

  mRecursive = new QCheckBox(tr("Recursive"), this);
  mRecursive->setChecked(true);

  mInit = new QCheckBox(tr("Init"), this);

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  buttons->addButton(QDialogButtonBox::Cancel);
  QPushButton *update =
      buttons->addButton(tr("Update"), QDialogButtonBox::AcceptRole);
  update->setEnabled(!model->enabledSubmodules().isEmpty());
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(model, &Model::dataChanged, [model, update] {
    update->setEnabled(!model->enabledSubmodules().isEmpty());
  });

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(mTable);
  layout->addWidget(mRecursive);
  layout->addWidget(mInit);
  layout->addWidget(buttons);
}

QList<git::Submodule> UpdateSubmodulesDialog::submodules() const {
  return static_cast<Model *>(mTable->model())->enabledSubmodules();
}

bool UpdateSubmodulesDialog::recursive() const {
  return mRecursive->isChecked();
}

bool UpdateSubmodulesDialog::init() const { return mInit->isChecked(); }
