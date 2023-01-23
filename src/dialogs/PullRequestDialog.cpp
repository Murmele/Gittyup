//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "PullRequestDialog.h"
#include "host/Account.h"
#include "ui/RepoView.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

PullRequestDialog::PullRequestDialog(RepoView *view) : QDialog(view) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Create Pull Request"));
  setMinimumWidth(400);

  mTitle = new QLineEdit(this);
  mTitle->setPlaceholderText(tr("Title"));

  mBody = new QTextEdit(this);
  mBody->setPlaceholderText(tr("Body"));

  setCommit(view->repo().head().target());

  QCheckBox *maintainerEdit = new QCheckBox(tr("Maintainer can modify"), this);
  maintainerEdit->setChecked(true);

  QComboBox *fromRepo = new QComboBox(this);
  fromRepo->setEditable(true);
  foreach (const git::Reference &ref, view->repo().branches(GIT_BRANCH_LOCAL))
    fromRepo->addItem(ref.name(), QVariant::fromValue(ref));
  fromRepo->setCurrentIndex(fromRepo->findText(view->repo().head().name()));
  auto indexChanged = QOverload<int>::of(&QComboBox::currentIndexChanged);
  connect(fromRepo, indexChanged, this, [this, fromRepo](int index) {
    setCommit(fromRepo->itemData(index).value<git::Reference>().target());
  });

  QVBoxLayout *fromLayout = new QVBoxLayout;
  fromLayout->addWidget(new QLabel(tr("From:"), this));
  fromLayout->addWidget(fromRepo);
  fromLayout->addStretch();

  Repository *remoteRepo = view->remoteRepo();

  QComboBox *toRepo = new QComboBox(this);
  toRepo->setEditable(true);
  toRepo->lineEdit()->setPlaceholderText(tr("owner/repository"));

  QComboBox *toBranch = new QComboBox(this);
  toBranch->setEditable(true);
  toBranch->lineEdit()->setPlaceholderText(tr("branch"));

  remoteRepo->account()->requestForkParents(remoteRepo);
  connect(remoteRepo->account(), &Account::forkParentsReady, this,
          [toRepo](const QMap<QString, QString> &parents) {
            foreach (const QString parent, parents.keys()) {
              toRepo->addItem(parent, parents.value(parent));
            }
          });
  connect(toRepo, indexChanged, this, [toRepo, toBranch](int index) {
    toBranch->clear();
    toBranch->addItem(toRepo->itemData(index).toString());
  });

  QVBoxLayout *toLayout = new QVBoxLayout;
  toLayout->addWidget(new QLabel(tr("To:")));
  toLayout->addWidget(toRepo);
  toLayout->addWidget(toBranch);

  QHBoxLayout *branches = new QHBoxLayout;
  branches->addLayout(fromLayout);
  branches->addLayout(toLayout);

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  buttons->addButton(QDialogButtonBox::Cancel);
  QPushButton *create =
      buttons->addButton(tr("Create"), QDialogButtonBox::AcceptRole);
  create->setDefault(true);
  create->setEnabled(false);

  auto signal = QOverload<const QString &>::of(&QComboBox::currentTextChanged);
  connect(toRepo, signal, [toRepo, toBranch, create]() {
    bool valid =
        !toRepo->currentText().isEmpty() && !toBranch->currentText().isEmpty();
    create->setEnabled(valid);
  });
  connect(toBranch, signal, [toRepo, toBranch, create]() {
    bool valid =
        !toRepo->currentText().isEmpty() && !toBranch->currentText().isEmpty();
    create->setEnabled(valid);
  });

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::close);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
  connect(create, &QPushButton::clicked,
          [this, remoteRepo, maintainerEdit, fromRepo, toRepo, toBranch] {
            remoteRepo->account()->createPullRequest(
                remoteRepo, toRepo->currentText(), mTitle->text(),
                mBody->toPlainText(), fromRepo->currentText(),
                toBranch->currentText(), maintainerEdit->isChecked());
          });

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(10);
  layout->addWidget(mTitle);
  layout->addWidget(mBody);
  layout->addWidget(maintainerEdit);
  layout->addLayout(branches);
  layout->addWidget(buttons);
}

void PullRequestDialog::setCommit(const git::Commit &commit) {
  mTitle->setText(commit.summary());

  QString body;
  if (!commit.body().isEmpty())
    body = QString("%1\n\n").arg(commit.body());

  mBody->setText(body);
}
