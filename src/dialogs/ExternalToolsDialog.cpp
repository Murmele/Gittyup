//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "ExternalToolsDialog.h"
#include "dialogs/ExternalToolsModel.h"
#include "git/Config.h"
#include "ui/Footer.h"
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

ExternalToolsDialog::ExternalToolsDialog(const QString &type, QWidget *parent)
    : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Configure External Tools"));
  setModal(false);
  resize(700, 600);

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::close);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);

  QVBoxLayout *externalToolsLayout = new QVBoxLayout(this);
  externalToolsLayout->setSpacing(10);
  externalToolsLayout->addLayout(createDetectedLayout(type));
  externalToolsLayout->addLayout(createUserDefinedLayout(type));
  externalToolsLayout->addWidget(buttons);
}

QVBoxLayout *ExternalToolsDialog::createDetectedLayout(const QString &type) {
  QLabel *title = new QLabel(tr("Detected Tools"));
  title->setStyleSheet("QLabel {padding: 0px 0px 5px 0px;}");

  QTableView *table = new QTableView(this);
  table->verticalHeader()->setVisible(false);
  table->setEditTriggers(QAbstractItemView::SelectedClicked);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::ExtendedSelection);
  table->setShowGrid(false);
  table->setModel(new ExternalToolsModel(type, true, this));
  table->horizontalHeader()->setSectionResizeMode(ExternalToolsModel::Arguments,
                                                  QHeaderView::Stretch);
  table->resizeColumnsToContents();
  table->selectRow(0);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(0);
  layout->addWidget(title);
  layout->addWidget(table);

  return layout;
}

QVBoxLayout *ExternalToolsDialog::createUserDefinedLayout(const QString &type) {
  QLabel *title = new QLabel(tr("User Defined Tools"));
  title->setStyleSheet("QLabel {padding: 0px 0px 5px 0px;}");

  ExternalToolsModel *model = new ExternalToolsModel(type, false, this);

  QTableView *table = new QTableView(this);
  table->verticalHeader()->setVisible(false);
  table->setEditTriggers(QAbstractItemView::SelectedClicked);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::ExtendedSelection);
  table->setShowGrid(false);
  table->setModel(model);
  table->sortByColumn(1, Qt::AscendingOrder);
  table->horizontalHeader()->setSectionResizeMode(ExternalToolsModel::Arguments,
                                                  QHeaderView::Stretch);
  table->resizeColumnsToContents();

  Footer *footer = new Footer(table);
  connect(footer, &Footer::plusClicked, [this, table, model] {
    model->add(QFileDialog::getOpenFileName(this, tr("Select Executable")));
    model->refresh();
    table->resizeColumnsToContents();
  });

  connect(footer, &Footer::minusClicked, [this, table, model] {
    QModelIndexList indexes = table->selectionModel()->selectedRows(0);
    foreach (const QModelIndex &index, indexes)
      model->remove(index.data(Qt::DisplayRole).toString());
    model->refresh();
    table->resizeColumnsToContents();
  });

  // Enable/disable minus button.
  auto updateMinusButton = [table, footer] {
    footer->setMinusEnabled(table->selectionModel()->hasSelection());
  };

  connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          updateMinusButton);
  connect(table->model(), &QAbstractItemModel::modelReset, this,
          updateMinusButton);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(0);
  layout->addWidget(title);
  layout->addWidget(table);
  layout->addWidget(footer);

  return layout;
}
