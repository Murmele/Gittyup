//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "NewBranchDialog.h"
#include "git/Reference.h"
#include "ui/ExpandButton.h"
#include "ui/ReferenceList.h"
#include "ui/RepoView.h"
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

NewBranchDialog::NewBranchDialog(const git::Repository &repo,
                                 const git::Commit &commit, QWidget *parent)
    : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);

  mName = new QLineEdit(this);

  auto kinds = ReferenceView::InvalidRef | ReferenceView::RemoteBranches;
  mUpstream = new ReferenceList(repo, kinds, this);

  kinds = ReferenceView::AllRefs;
  if (commit.isValid())
    kinds |= ReferenceView::InvalidRef;
  mRefs = new ReferenceList(repo, kinds, this);
  mRefs->select(repo.head());
  mRefs->setCommit(commit);
  mRefs->setVisible(!commit.isValid());

  mCheckout = new QCheckBox(tr("Checkout branch"), this);
  mCheckout->setVisible(qobject_cast<RepoView *>(parent));
  mCheckout->setChecked(true);

  ExpandButton *expand = new ExpandButton(this);
  QFormLayout *form = new QFormLayout;
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->addRow(tr("Name:"), mName);
  if (!commit.isValid()) {
    form->addRow(tr("Start Point:"), mRefs);
  }
  form->addRow(QString(), mCheckout);
  form->addRow(tr("Advanced:"), expand);

  QWidget *advanced = new QWidget(this);
  advanced->setVisible(false);

  QFormLayout *advancedForm = new QFormLayout(advanced);
  advancedForm->setContentsMargins(-1, 0, 0, 0);
  advancedForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  advancedForm->addRow(tr("Upstream:"), mUpstream);

  connect(expand, &ExpandButton::toggled, [this, advanced](bool checked) {
    advanced->setVisible(checked);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    resize(sizeHint());
  });

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  buttons->addButton(QDialogButtonBox::Cancel);
  QPushButton *create =
      buttons->addButton(tr("Create Branch"), QDialogButtonBox::AcceptRole);
  create->setEnabled(false);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(advanced);
  layout->addWidget(buttons);

  // Update button when name text changes.
  connect(mName, &QLineEdit::textChanged, [repo, create](const QString &text) {
    create->setEnabled(git::Branch::isNameValid(text) &&
                       !repo.lookupBranch(text, GIT_BRANCH_LOCAL).isValid());
  });

  // Populate name and start point when upstream changes.
  connect(mUpstream, &ReferenceList::referenceSelected,
          [this](const git::Reference &ref) {
            if (ref.isValid()) {
              if (mName->text().isEmpty())
                mName->setText(ref.name().section('/', -1));
              mRefs->select(ref);
            }
          });
}

QString NewBranchDialog::name() const { return mName->text(); }

bool NewBranchDialog::checkout() const { return mCheckout->isChecked(); }

git::Commit NewBranchDialog::target() const { return mRefs->target(); }

git::Reference NewBranchDialog::upstream() const {
  return mUpstream->currentReference();
}
