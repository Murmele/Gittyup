//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#include "AddRemoteDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

AddRemoteDialog::AddRemoteDialog(const QString &name, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(tr("Add Remote"));

  mName = new QLineEdit(name, this);
  connect(mName, &QLineEdit::textChanged, this, &AddRemoteDialog::update);

  mUrl = new QLineEdit(this);
  connect(mUrl, &QLineEdit::textChanged, this, &AddRemoteDialog::update);

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  buttons->addButton(QDialogButtonBox::Cancel);
  mAdd = buttons->addButton(tr("Add Remote"), QDialogButtonBox::AcceptRole);

  QFormLayout *layout = new QFormLayout(this);
  layout->addRow(tr("Name:"), mName);
  layout->addRow(tr("URL:"), mUrl);
  layout->addRow(buttons);

  update();
}

QString AddRemoteDialog::name() const { return mName->text(); }

QString AddRemoteDialog::url() const { return mUrl->text(); }

void AddRemoteDialog::update(const QString &text) {
  mAdd->setEnabled(!name().isEmpty() && !url().isEmpty());
}
