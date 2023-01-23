//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CommitDialog.h"

#include "conf/Settings.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

CommitDialog::CommitDialog(const QString &message, Prompt::Kind kind,
                           QWidget *parent)
    : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);

  QString title;
  switch (kind) {
    case Prompt::Kind::Merge:
      title = tr("Merge commit message");
      break;

    case Prompt::Kind::Stash:
      title = tr("Stash commit message");
      break;

    case Prompt::Kind::Revert:
      title = tr("Revert commit message");
      break;

    case Prompt::Kind::CherryPick:
      title = tr("Cherry-pick commit message");
      break;

    case Prompt::Kind::Directories:
    case Prompt::Kind::LargeFiles:
      Q_ASSERT(false);
      break;
  }

  setWindowTitle(title);
  QLabel *label = new QLabel(QString("<b>%1:</b>").arg(title), this);

  mEditor = new QTextEdit(this);
  mEditor->setFixedWidth(400);
  mEditor->setFixedHeight(120);
  mEditor->setText(message);

  Settings *settings = Settings::instance();
  QCheckBox *prompt = new QCheckBox(settings->promptDescription(kind), this);
  prompt->setChecked(settings->prompt(kind));
  connect(prompt, &QCheckBox::toggled, this, [kind](bool checked) {
    Settings::instance()->setPrompt(kind, checked);
  });

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  switch (kind) {
    case Prompt::Kind::Merge:
      buttons->addButton(tr("Merge"), QDialogButtonBox::AcceptRole);
      buttons->addButton(tr("Abort"), QDialogButtonBox::RejectRole);
      break;

    case Prompt::Kind::Stash:
      buttons->addButton(tr("Stash"), QDialogButtonBox::AcceptRole);
      buttons->addButton(QDialogButtonBox::Cancel);
      break;

    case Prompt::Kind::Revert:
      buttons->addButton(tr("Revert"), QDialogButtonBox::AcceptRole);
      buttons->addButton(tr("Abort"), QDialogButtonBox::RejectRole);
      break;

    case Prompt::Kind::CherryPick:
      buttons->addButton(tr("Cherry-pick"), QDialogButtonBox::AcceptRole);
      buttons->addButton(tr("Abort"), QDialogButtonBox::RejectRole);
      break;

    case Prompt::Kind::Directories:
    case Prompt::Kind::LargeFiles:
      Q_ASSERT(false);
      break;
  }

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addWidget(label);
  layout->addWidget(mEditor);
  layout->addWidget(prompt);
  layout->addWidget(buttons);

  // Start with focus on the accept button.
  buttons->setFocus();
}

QString CommitDialog::message() const { return mEditor->toPlainText(); }

void CommitDialog::open() {
  QDialog::open();
  mEditor->clearFocus();
}
