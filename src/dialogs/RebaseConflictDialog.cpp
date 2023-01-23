//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Kas
//

#include "RebaseConflictDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

RebaseConflictDialog::RebaseConflictDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle(tr("Rebase conflict"));

  QDialogButtonBox *buttons = new QDialogButtonBox(this);

  connect(buttons->addButton(tr("Abort rebase"), QDialogButtonBox::AcceptRole),
          &QPushButton::clicked, [this](bool checked) {
            mUserChoice = ChosenAction::Abort;
            accept();
          });

  connect(buttons->addButton(tr("Continue"), QDialogButtonBox::AcceptRole),
          &QPushButton::clicked, [this](bool checked) {
            mUserChoice = ChosenAction::Fix;
            accept();
          });

  QFormLayout *layout = new QFormLayout(this);
  layout->addRow(
      new QLabel(tr("The rebase caused a merge conflict. \n"
                    "Would you like to fix the merge conflict and continue?"),
                 this));
  layout->addRow(buttons);
}
