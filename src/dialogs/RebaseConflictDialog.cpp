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

RebaseConflictDialog::RebaseConflictDialog(QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Rebase conflict"));
  
  QDialogButtonBox *buttons = new QDialogButtonBox(this);

  connect(
    buttons->addButton(tr("Abort rebase"), QDialogButtonBox::AcceptRole),
    &QPushButton::clicked,
    [this](bool checked) {
      mUserChoice = ChosenAction::Abort;
      accept();
    }
  );

  connect(
    buttons->addButton(tr("Keep rebase"), QDialogButtonBox::AcceptRole),
    &QPushButton::clicked,
    [this](bool checked) {
      mUserChoice = ChosenAction::Leave;
      accept();
    }
  );

  QFormLayout *layout = new QFormLayout(this);
  layout->addRow(new QLabel(tr(
    "The rebase caused a merge conflict. \n"
    "As Gittyup currently doesn't support working on rebase conflicts, \n"
    "you have the option to continue solving the conflict using the command line"
  ), this));
  layout->addRow(buttons);
}
