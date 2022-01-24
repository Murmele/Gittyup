//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Kas
//

#ifndef REBASECONFLICTDIALOG_H
#define REBASECONFLICTDIALOG_H

#include <QDialog>

class RebaseConflictDialog : public QDialog
{
  Q_OBJECT

public:
  enum class ChosenAction
  {
    Unset,
    Abort,
    Leave
  };

  RebaseConflictDialog(QWidget *parent = nullptr);

  inline ChosenAction userChoice() const
  {
    return mUserChoice;
  }

private:
  ChosenAction mUserChoice = ChosenAction::Unset;
};

#endif

