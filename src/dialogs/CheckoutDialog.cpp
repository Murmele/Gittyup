//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CheckoutDialog.h"
#include "git/Branch.h"
#include "ui/ReferenceList.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

CheckoutDialog::CheckoutDialog(const git::Repository &repo,
                               const git::Reference &ref, QWidget *parent)
    : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);

  mRefs = new ReferenceList(repo, ReferenceView::AllRefs, this);
  connect(mRefs, &ReferenceList::referenceSelected, this,
          &CheckoutDialog::update);

  mDetachBox = new QCheckBox(tr("Detach HEAD"), this);
  connect(mDetachBox, &QCheckBox::toggled, [this](bool checked) {
    if (mDetachBox->isEnabled()) {
      mDetach = checked;
      update(mRefs->currentReference());
    }
  });

  QFormLayout *form = new QFormLayout;
  form->addRow(tr("References:"), mRefs);
  form->addRow(QString(), mDetachBox);

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  buttons->addButton(QDialogButtonBox::Cancel);
  mCheckout = buttons->addButton(tr("Checkout"), QDialogButtonBox::AcceptRole);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(buttons);

  mRefs->select(ref);
  update(mRefs->currentReference());
}

git::Reference CheckoutDialog::reference() const {
  return mRefs->currentReference();
}

void CheckoutDialog::update(const git::Reference &ref) {
  if (!ref.isValid()) {
    mDetachBox->setEnabled(false);
    mCheckout->setEnabled(false);
    return;
  }

  bool local = ref.isLocalBranch();
  mDetachBox->setEnabled(local);
  mDetachBox->setChecked(!mDetachBox->isEnabled() || mDetach);
  mCheckout->setEnabled(!ref.isHead() || (local && mDetachBox->isChecked()));
}
