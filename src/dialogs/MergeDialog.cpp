//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#include "MergeDialog.h"
#include "conf/Settings.h"
#include "git/Branch.h"
#include "ui/ReferenceList.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const ReferenceView::Kinds kRefKinds =
    ReferenceView::InvalidRef | ReferenceView::LocalBranches |
    ReferenceView::RemoteBranches | ReferenceView::Tags |
    ReferenceView::ExcludeHead;

} // namespace

MergeDialog::MergeDialog(RepoView::MergeFlags flags,
                         const git::Repository &repo, QWidget *parent)
    : QDialog(parent), mRepo(repo) {
  setAttribute(Qt::WA_DeleteOnClose);

  mRefs = new ReferenceList(repo, kRefKinds, this);
  connect(mRefs, &ReferenceList::referenceSelected, this, &MergeDialog::update);

  auto noff = RepoView::Merge | RepoView::NoFastForward;
  auto ffonly = RepoView::Merge | RepoView::FastForward;

  mAction = new QComboBox(this);
  mAction->addItem(tr("Merge"), RepoView::Merge);
  mAction->addItem(tr("Rebase"), RepoView::Rebase);
  mAction->addItem(tr("Squash"), RepoView::Squash);
  mAction->addItem(tr("Merge (No Fast-forward)"), noff);
  mAction->addItem(tr("Merge (Fast-forward Only)"), ffonly);
  mAction->setCurrentIndex(mAction->findData(static_cast<int>(flags)));

  QLabel *label = new QLabel(labelText(), this);

  QCheckBox *noCommit = new QCheckBox(tr("No commit"), this);
  noCommit->setChecked(!Settings::instance()
                            ->value(Setting::Id::CommitMergeImmediately)
                            .toBool());
  connect(noCommit, &QCheckBox::toggled, [](bool checked) {
    Settings::instance()->setValue(Setting::Id::CommitMergeImmediately,
                                   !checked);
  });

  noCommit->setVisible(flags & RepoView::Merge);

  auto signal = QOverload<int>::of(&QComboBox::currentIndexChanged);
  connect(mAction, signal, [this, label, noCommit]() {
    RepoView::MergeFlags flags = this->flags();
    bool merge = (flags & RepoView::Merge);
    bool ffonly = (flags & RepoView::FastForward);

    label->setText(labelText());
    mAccept->setText(buttonText());
    noCommit->setVisible(merge && !ffonly);
  });

  QFormLayout *form = new QFormLayout;
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->addRow(label);
  form->addRow(tr("Reference:"), mRefs);
  form->addRow(tr("Action:"), mAction);
  form->addRow(QString(), noCommit);

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  buttons->addButton(QDialogButtonBox::Cancel);
  mAccept = buttons->addButton(buttonText(), QDialogButtonBox::AcceptRole);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(buttons);

  update();
}

git::Commit MergeDialog::target() const { return mRefs->target(); }

git::Reference MergeDialog::reference() const {
  return mRefs->currentReference();
}

RepoView::MergeFlags MergeDialog::flags() const {
  int action = mAction->itemData(mAction->currentIndex()).toInt();
  RepoView::MergeFlags flags = static_cast<RepoView::MergeFlags>(action);
  if (!Settings::instance()
           ->value(Setting::Id::CommitMergeImmediately)
           .toBool())
    flags |= RepoView::NoCommit;
  return flags;
}

void MergeDialog::setCommit(const git::Commit &commit) {
  mRefs->setCommit(commit);
  update();
}

void MergeDialog::setReference(const git::Reference &ref) {
  mRefs->select(ref);
  update();
}

void MergeDialog::update() { mAccept->setEnabled(mRefs->target().isValid()); }

QString MergeDialog::labelText() const {
  QString fmt;
  if (flags() & RepoView::Merge)
    fmt = tr("Choose a reference to merge into '%1'.");
  else if (flags() & RepoView::Rebase)
    fmt = tr("Choose a reference to rebase '%1' on.");
  else
    fmt = tr("Choose a reference to squash into '%1'.");

  git::Reference head = mRepo.head();
  Q_ASSERT(head.isValid());

  return fmt.arg(head.name(false));
}

QString MergeDialog::buttonText() const {
  if (flags() & RepoView::Merge)
    return tr("Merge");

  return (flags() & RepoView::Rebase) ? tr("Rebase") : tr("Squash");
}
