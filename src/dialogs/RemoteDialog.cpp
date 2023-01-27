//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RemoteDialog.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "git/Repository.h"
#include "ui/ExpandButton.h"
#include "ui/ReferenceList.h"
#include "ui/RepoView.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

RemoteDialog::RemoteDialog(Kind kind, RepoView *parent) : QDialog(parent) {
  git::Repository repo = parent->repo();
  setAttribute(Qt::WA_DeleteOnClose);

  mRemotes = new QComboBox(this);
  mRemotes->setEditable(true);
  mRemotes->setMinimumContentsLength(16);
  foreach (const git::Remote &remote, repo.remotes())
    mRemotes->addItem(remote.name(), QVariant::fromValue(remote));

  git::Remote defaultRemote = repo.defaultRemote();
  if (defaultRemote.isValid()) {
    int index = mRemotes->findText(defaultRemote.name());
    if (index >= 0)
      mRemotes->setCurrentIndex(index);
  }

  QString tagsText =
      (kind == Push) ? tr("Push all tags") : tr("Update existing tags");
  mTags = new QCheckBox(tagsText, this);

  if (kind == Pull) {
    auto noff = RepoView::Merge | RepoView::NoFastForward;
    auto ffonly = RepoView::Merge | RepoView::FastForward;

    mAction = new QComboBox(this);
    mAction->addItem(tr("Merge"), RepoView::Merge);
    mAction->addItem(tr("Rebase"), RepoView::Rebase);
    mAction->addItem(tr("Merge (No Fast-forward)"), noff);
    mAction->addItem(tr("Merge (Fast-forward Only)"), ffonly);
  }

  QWidget *advanced = nullptr;
  ExpandButton *expand = nullptr;
  QCheckBox *prune = nullptr;

  if (kind == Push) {
    auto kinds = ReferenceView::LocalBranches | ReferenceView::Tags;
    mRefs = new ReferenceList(repo, kinds, this);
    mSetUpstream = new QCheckBox(tr("Set upstream"), this);
    mForce = new QCheckBox(tr("Force"), this);

    // advanced options
    expand = new ExpandButton(this);

    advanced = new QWidget(this);
    advanced->setVisible(false);

    mRemoteRef = new QLineEdit(advanced);

    QFormLayout *advancedForm = new QFormLayout(advanced);
    advancedForm->setContentsMargins(-1, 0, 0, 0);
    advancedForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    advancedForm->addRow(tr("Remote Reference:"), mRemoteRef);

    connect(expand, &ExpandButton::toggled, [this, advanced](bool checked) {
      advanced->setVisible(checked);
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      resize(sizeHint());
    });

    connect(mRefs, &ReferenceList::referenceSelected,
            [this](const git::Reference &ref) {
              QString value = QString();
              if (ref.isValid()) {
                QString key = QString("branch.%1.merge").arg(ref.name());
                git::Config config =
                    RepoView::parentView(this)->repo().gitConfig();
                value = config.value<QString>(key);
              }
              mRemoteRef->setText(value);
            });

    mRefs->select(repo.head());

  } else {
    bool autoPrune =
        Settings::instance()->value(Setting::Id::PruneAfterFetch).toBool();
    git::Config config = repo.appConfig();

    prune = new QCheckBox(tr("Prune references"), this);
    prune->setChecked(config.value<bool>("autoprune.enable", autoPrune));
  }

  QString button;
  switch (kind) {
    case Fetch:
      button = tr("Fetch");
      break;

    case Pull:
      button = tr("Pull");
      break;

    case Push:
      button = tr("Push");
      break;
  }

  QFormLayout *form = new QFormLayout;
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->addRow(tr("Remote:"), mRemotes);
  if (mRefs)
    form->addRow(tr("Reference:"), mRefs);
  if (mAction)
    form->addRow(tr("Action:"), mAction);
  form->addRow(QString(), mTags);
  if (prune)
    form->addRow(QString(), prune);
  if (mSetUpstream)
    form->addRow(QString(), mSetUpstream);
  if (mForce)
    form->addRow(QString(), mForce);
  if (expand)
    form->addRow(tr("Advanced:"), expand);

  QDialogButtonBox *buttons =
      new QDialogButtonBox(QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  buttons->addButton(button, QDialogButtonBox::AcceptRole);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  if (advanced)
    layout->addWidget(advanced);
  layout->addWidget(buttons);

  connect(this, &RemoteDialog::accepted, [this, kind, prune] {
    RepoView *view = RepoView::parentView(this);
    QString remoteName = mRemotes->currentText();
    git::Remote tmp = mRemotes->currentData().value<git::Remote>();
    git::Remote remote = (tmp.isValid() && tmp.name() == remoteName)
                             ? tmp
                             : view->repo().anonymousRemote(remoteName);
    bool tags = mTags->isChecked();

    switch (kind) {
      case Fetch:
        view->fetch(remote, tags, true, nullptr, nullptr, prune->isChecked());
        break;

      case Pull: {
        RepoView::MergeFlags flags(mAction->currentData().toInt());
        view->pull(flags, remote, tags, prune->isChecked());
        break;
      }

      case Push: {
        git::Reference ref = mRefs->currentReference();
        QString remoteRef = mRemoteRef->text();
        bool setUpstream = mSetUpstream->isChecked();
        bool force = mForce->isChecked();
        view->push(remote, ref, remoteRef, setUpstream, force, tags);
        break;
      }
    }
  });
}
