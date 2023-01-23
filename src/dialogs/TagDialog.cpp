//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include <algorithm>

#include "TagDialog.h"
#include "git/Repository.h"
#include "git/TagRef.h"
#include "git2/tag.h"
#include "ui/ExpandButton.h"
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QStringList>

TagDialog::TagDialog(const git::Repository &repo, const QString &id,
                     const git::Remote &remote, QWidget *parent)
    : QDialog(parent), mRemote(remote) {
  setAttribute(Qt::WA_DeleteOnClose);

  setWindowTitle(tr("Create Tag"));
  QString text = tr("Add a new tag at %1").arg(id);
  QLabel *label = new QLabel(QString("<b>%1:</b>").arg(text), this);

  mNameField = new QLineEdit(this);
  mForce = new QCheckBox(tr("Force (replace existing tag)"), this);

  if (remote.isValid())
    mPush = new QCheckBox(tr("Push to %1").arg(remote.name()), this);
  else
    mPush = nullptr;

  QCheckBox *annotated = new QCheckBox(tr("Annotated"), this);
  ExpandButton *expand = new ExpandButton(this);
  connect(annotated, &QCheckBox::toggled, expand, &ExpandButton::setChecked);

  QHBoxLayout *annotatedLayout = new QHBoxLayout;
  annotatedLayout->addWidget(annotated);
  annotatedLayout->addWidget(expand);
  annotatedLayout->addStretch();

  mMessage = new QTextEdit(this);
  mMessage->setEnabled(false);
  mMessage->setVisible(false);
  connect(annotated, &QCheckBox::toggled, mMessage, &QTextEdit::setEnabled);
  connect(expand, &ExpandButton::toggled, [this](bool checked) {
    mMessage->setVisible(checked);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    resize(minimumSizeHint());
  });

  QDialogButtonBox *buttons =
      new QDialogButtonBox(QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QPushButton *create =
      buttons->addButton(tr("Create Tag"), QDialogButtonBox::AcceptRole);
  create->setEnabled(false);

  // filtering must be done only once, because mExistingTags does not change
  // during the use of this dialog
  mExistingTags = repo.existingTags(),
  // filter descending. V1.1 before V1.0 makes more sense, because normaly the
  // next greater number will be choosen and so it can be seen faster
      std::sort(mExistingTags.begin(), mExistingTags.end(), std::greater<>());
  mFilteredTags = mExistingTags;

  mListWidget = new QListWidget(this);
  mListWidget->addItems(mExistingTags);

  connect(mListWidget, &QListWidget::itemDoubleClicked,
          [this](QListWidgetItem *item) { mNameField->setText(item->text()); });

  auto updateButton = [this, repo, create, annotated] {
    QString name = mNameField->text();
    int value = QString::compare(name, mOldTagname);
    mOldTagname = name;
    bool force = mForce->isChecked();
    create->setEnabled(
        !name.isEmpty() && (force || !repo.lookupTag(name).isValid()) &&
        (!annotated->isChecked() || !mMessage->toPlainText().isEmpty()));

    if (value >= 0) {
      // a new character was added, so the filtered data can be filtered again
      mFilteredTags =
          mFilteredTags.filter(name, Qt::CaseSensitivity::CaseSensitive);
    } else {
      mFilteredTags =
          mExistingTags.filter(name, Qt::CaseSensitivity::CaseSensitive);
    }

    mListWidget->clear();
    mListWidget->addItems(mFilteredTags);
  };

  connect(mNameField, &QLineEdit::textChanged, updateButton);
  connect(mForce, &QCheckBox::toggled, updateButton);
  connect(annotated, &QCheckBox::toggled, updateButton);
  connect(mMessage, &QTextEdit::textChanged, updateButton);

  QFormLayout *layout = new QFormLayout();
  layout->addRow(label);
  layout->addRow(tr("Name"), mNameField);
  layout->addRow(mForce);
  if (mPush)
    layout->addRow(mPush);
  layout->addRow(annotatedLayout);
  layout->addRow(mMessage);

  QHBoxLayout *hLayout = new QHBoxLayout();
  hLayout->addLayout(layout);

  QVBoxLayout *vLayout = new QVBoxLayout();
  vLayout->addWidget(new QLabel("Existing Tags:", this));
  vLayout->addWidget(mListWidget);
  hLayout->addLayout(vLayout);

  vLayout = new QVBoxLayout();
  vLayout->addLayout(hLayout);
  vLayout->addWidget(buttons);

  setLayout(vLayout);
}

bool TagDialog::force() const { return mForce->isChecked(); }

git::Remote TagDialog::remote() const {
  if (mPush && mPush->isChecked())
    return mRemote;
  else
    return git::Remote();
}

QString TagDialog::name() const { return mNameField->text(); }

QString TagDialog::message() const { return mMessage->toPlainText(); }
