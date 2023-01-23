//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "UpToDateDialog.h"
#include "dialogs/IconLabel.h"
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

UpToDateDialog::UpToDateDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle(tr("Already Up-to-date"));

  QIcon icon(":/Gittyup.iconset/icon_128x128.png");
  IconLabel *iconLabel = new IconLabel(icon, 64, 64, this);

  QVBoxLayout *iconLayout = new QVBoxLayout;
  iconLayout->addWidget(iconLabel);
  iconLayout->addStretch();

  QString name = QCoreApplication::applicationName();
  QString version = QCoreApplication::applicationVersion();
  QString text = tr("%1 is already up-to-date. You have version %2.");
  QLabel *label = new QLabel(text.arg(name, version), this);
  label->setWordWrap(true);

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &UpToDateDialog::accept);

  QVBoxLayout *content = new QVBoxLayout;
  content->addWidget(label);
  content->addWidget(buttons);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addLayout(iconLayout);
  layout->addLayout(content);
}
