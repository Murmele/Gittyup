// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Michael WERLE
//

#include "RenameBranchDialog.h"
#include "git/Branch.h"
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

RenameBranchDialog::RenameBranchDialog(const git::Repository &repo,
                                       const git::Branch &branch,
                                       QWidget *parent)
    : QDialog(parent) {
  Q_ASSERT(branch.isValid() && branch.isLocalBranch());
  setAttribute(Qt::WA_DeleteOnClose);

  mName = new QLineEdit(branch.name(), this);
  mName->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  mName->setMinimumWidth(QFontMetrics(mName->font()).averageCharWidth() * 40);

  QFormLayout *form = new QFormLayout;
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->addRow(tr("Name:"), mName);

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  buttons->addButton(QDialogButtonBox::Cancel);
  QPushButton *rename =
      buttons->addButton(tr("Rename Branch"), QDialogButtonBox::AcceptRole);
  rename->setEnabled(false);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(buttons);

  // Update button when name text changes.
  connect(mName, &QLineEdit::textChanged, [repo, rename](const QString &text) {
    rename->setEnabled(git::Branch::isNameValid(text) &&
                       !repo.lookupBranch(text, GIT_BRANCH_LOCAL).isValid());
  });

  // Perform the rename when the button is clicked
  connect(rename, &QPushButton::clicked,
          [this, branch] { git::Branch(branch).rename(mName->text()); });
}

QString RenameBranchDialog::name() const { return mName->text(); }
