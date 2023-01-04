
#include "FileWidget.h"
#include "DisclosureButton.h"
#include "EditButton.h"
#include "DiscardButton.h"
#include "LineStats.h"
#include "FileLabel.h"
#include "HunkWidget.h"
#include "Images.h"
#include "conf/Settings.h"
#include "git/Commit.h"
#include "git/Patch.h"
#include "git2/checkout.h"
#include "git2/diff.h"
#include "ui/RepoView.h"
#include "ui/Badge.h"
#include "ui/FileContextMenu.h"
#include "git/Repository.h"

#include "git/Buffer.h"

#include <QCheckBox>
#include <QContextMenuEvent>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>

namespace {
bool disclosure = false;
}

_FileWidget::Header::Header(const git::Diff &diff, git::Patch &patch,
                            bool binary, bool lfs, bool submodule,
                            QWidget *parent)
    : QFrame(parent), mDiff(diff), mPatch(patch), mSubmodule(submodule) {
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  QString name = patch.name();
  mCheck = new QCheckBox(this);
  mCheck->setVisible(diff.isStatusDiff());
  mCheck->setTristate(true);

  mStatusBadge = new Badge({}, this);

  git::Patch::LineStats lineStats;
  lineStats.additions = 0;
  lineStats.deletions = 0;
  mStats = new LineStats(lineStats, this);
  mStats->setVisible(false);

  mFileLabel = new FileLabel(name, submodule, this);

  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->setContentsMargins(0, 0, 0, 0);
  buttons->setSpacing(4);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->addWidget(mCheck);
  layout->addSpacing(4);
  layout->addWidget(mStatusBadge);
  layout->addWidget(mStats);
  layout->addWidget(mFileLabel, 1);
  layout->addStretch();
  layout->addLayout(buttons);

  // Add LFS buttons.
  if (lfs) {
    Badge *lfsBadge =
        new Badge({Badge::Label(FileWidget::tr("LFS"), true)}, this);
    buttons->addWidget(lfsBadge);

    QToolButton *lfsLockButton = new QToolButton(this);
    bool locked = patch.repo().lfsIsLocked(patch.name());
    lfsLockButton->setText(locked ? FileWidget::tr("Unlock")
                                  : FileWidget::tr("Lock"));
    buttons->addWidget(lfsLockButton);

    connect(lfsLockButton, &QToolButton::clicked, [this, patch] {
      bool locked = patch.repo().lfsIsLocked(patch.name());
      RepoView::parentView(this)->lfsSetLocked({patch.name()}, !locked);
    });

    git::RepositoryNotifier *notifier = patch.repo().notifier();
    connect(notifier, &git::RepositoryNotifier::lfsLocksChanged, this,
            [this, patch, lfsLockButton] {
              bool locked = patch.repo().lfsIsLocked(patch.name());
              lfsLockButton->setText(locked ? FileWidget::tr("Unlock")
                                            : FileWidget::tr("Lock"));
            });

    mLfsButton = new QToolButton(this);
    mLfsButton->setText(FileWidget::tr("Show Object"));
    mLfsButton->setCheckable(true);

    buttons->addWidget(mLfsButton);
    buttons->addSpacing(8);
  }

  // Add edit button.
  mEdit = new EditButton(patch, -1, binary, lfs, this);
  mEdit->setToolTip(FileWidget::tr("Edit File"));
  buttons->addWidget(mEdit);

  // Add discard button.
  mDiscardButton = new DiscardButton(this);
  mDiscardButton->setVisible(false);
  mDiscardButton->setToolTip(FileWidget::tr("Discard File"));
  buttons->addWidget(mDiscardButton);
  connect(mDiscardButton, &QToolButton::clicked, this,
          &_FileWidget::Header::discard);

  mDisclosureButton = new DisclosureButton(this);
  mDisclosureButton->setToolTip(mDisclosureButton->isChecked()
                                    ? FileWidget::tr("Collapse File")
                                    : FileWidget::tr("Expand File"));
  connect(mDisclosureButton, &DisclosureButton::toggled, [this] {
    mDisclosureButton->setToolTip(mDisclosureButton->isChecked()
                                      ? FileWidget::tr("Collapse File")
                                      : FileWidget::tr("Expand File"));
  });
  mDisclosureButton->setVisible(disclosure);
  buttons->addWidget(mDisclosureButton);

  mSave = new QToolButton(this);
  mSave->setObjectName("ConflictFileSave");
  mSave->setText(HunkWidget::tr("Save"));

  mUndo = new QToolButton(this);
  mUndo->setObjectName("ConflictFileUndo");
  mUndo->setText(HunkWidget::tr("Undo"));
  connect(mUndo, &QToolButton::clicked, [this] {
    mSave->setVisible(false);
    mUndo->setVisible(false);
    mOurs->setEnabled(true);
    mTheirs->setEnabled(true);
    mResolution = git::Patch::ConflictResolution::Unresolved;
  });

  mOurs = new QToolButton(this);
  mOurs->setObjectName("ConflictFileOurs");
  mOurs->setStyleSheet(
      Application::theme()->diffButtonStyle(Theme::Diff::Ours));
  connect(mOurs, &QToolButton::clicked, [this] {
    mSave->setVisible(true);
    mUndo->setVisible(true);
    mOurs->setEnabled(false);
    mTheirs->setEnabled(false);
    mResolution = git::Patch::ConflictResolution::Ours;
  });

  mTheirs = new QToolButton(this);
  mTheirs->setObjectName("ConflictFileTheirs");
  mTheirs->setStyleSheet(
      Application::theme()->diffButtonStyle(Theme::Diff::Theirs));
  connect(mTheirs, &QToolButton::clicked, [this] {
    mSave->setVisible(true);
    mUndo->setVisible(true);
    mOurs->setEnabled(false);
    mTheirs->setEnabled(false);
    mResolution = git::Patch::ConflictResolution::Theirs;
  });

  buttons->addWidget(mSave);
  buttons->addWidget(mUndo);
  buttons->addWidget(mOurs);
  buttons->addWidget(mTheirs);

  updatePatch(patch);

  if (!diff.isStatusDiff())
    return;

  // Respond to check changes.
  connect(mCheck, &QCheckBox::clicked, [this](bool staged) {
    if (staged)
      emit stageStateChanged(Qt::Checked);
    else
      emit stageStateChanged(Qt::Unchecked);
  });

  // Set initial check state.
  updateCheckState();
}

void _FileWidget::Header::updatePatch(git::Patch &patch) {
  auto status = patch.status();
  QList<Badge::Label> labels = {
      Badge::Label(QChar(git::Diff::statusChar(status)))};

  git::Patch::LineStats lineStats = patch.lineStats();
  mStats->setStats(lineStats);
  mStats->setVisible(lineStats.additions > 0 || lineStats.deletions > 0);

  mFileLabel->setName(patch.name());
  if (status == GIT_DELTA_RENAMED)
    mFileLabel->setOldName(patch.name(git::Diff::OldFile));

  mEdit->updatePatch(patch, -1);

  auto isConflicted = status == GIT_DELTA_CONFLICTED;
  auto showFileSolverButtons = patch.count() != 1;

  if (isConflicted) {
    auto conflict = mDiff.index().conflict(patch.name());
    auto ours = QString();
    auto theirs = QString();

    mOurs->setText(HunkWidget::tr("Use Ours"));
    mTheirs->setText(HunkWidget::tr("Use Theirs"));

    if (conflict.ancestor.isNull()) {
      if (!conflict.ours.isNull()) {
        ours = "A";

        if (conflict.theirs.isNull()) {
          mTheirs->setText(tr("Use Theirs: Delete"));
        }
      }

      if (!conflict.theirs.isNull()) {
        theirs = "A";

        if (conflict.ours.isNull()) {
          mOurs->setText(tr("Use Ours: Delete"));
        }
      }

    } else {
      if (conflict.ours.isNull() && conflict.theirs.isNull()) {
        showFileSolverButtons = false;
      }

      if (conflict.ours.isNull()) {
        ours = "D";
        mOurs->setText(tr("Use Ours: Delete"));
      } else if (conflict.ours != conflict.ancestor) {
        ours = "M";
      }

      if (conflict.theirs.isNull()) {
        theirs = "D";
        mTheirs->setText(tr("Use Theirs: Delete"));
      } else if (conflict.theirs != conflict.ancestor) {
        theirs = "M";
      }
    }

    if (!ours.isEmpty() && ours == theirs) {
      labels.append(Badge::Label(tr("both: %1").arg(ours)));
    } else {
      if (!ours.isEmpty()) {
        labels.append(Badge::Label(tr("ours: %1").arg(ours)));
      }
      if (!theirs.isEmpty()) {
        labels.append(Badge::Label(tr("theirs: %1").arg(theirs)));
      }
    }
  }

  mStatusBadge->setLabels(labels);

  mOurs->setVisible(isConflicted && showFileSolverButtons);
  mTheirs->setVisible(isConflicted && showFileSolverButtons);

  mSave->setVisible(mResolution != git::Patch::ConflictResolution::Unresolved);
  mUndo->setVisible(mResolution != git::Patch::ConflictResolution::Unresolved);
  mOurs->setEnabled(mResolution == git::Patch::ConflictResolution::Unresolved);
  mTheirs->setEnabled(mResolution ==
                      git::Patch::ConflictResolution::Unresolved);

  mDiscardButton->setVisible(mDiff.isStatusDiff() && !mSubmodule &&
                             !isConflicted);
}
QCheckBox *_FileWidget::Header::check() const { return mCheck; }

DisclosureButton *_FileWidget::Header::disclosureButton() const {
  return mDisclosureButton;
}

QToolButton *_FileWidget::Header::lfsButton() const { return mLfsButton; }

void _FileWidget::Header::setStageState(git::Index::StagedState state) {
  if (state == git::Index::Staged)
    mCheck->setCheckState(Qt::Checked);
  else if (state == git::Index::Unstaged)
    mCheck->setCheckState(Qt::Unchecked);
  else
    mCheck->setCheckState(Qt::PartiallyChecked);
}

void _FileWidget::Header::mouseDoubleClickEvent(QMouseEvent *event) {
  if (mDisclosureButton->isEnabled())
    mDisclosureButton->toggle();
}

void _FileWidget::Header::contextMenuEvent(QContextMenuEvent *event) {
  RepoView *view = RepoView::parentView(this);
  FileContextMenu menu(view, {mPatch.name()}, mDiff.index());
  menu.exec(event->globalPos());
}

void _FileWidget::Header::updateCheckState() {
  bool disabled = false;
  Qt::CheckState state = Qt::Unchecked;
  switch (mDiff.index().isStaged(mPatch.name())) {
    case git::Index::Disabled:
      disabled = true;
      break;

    case git::Index::Unstaged:
      break;

    case git::Index::PartiallyStaged:
      state = Qt::PartiallyChecked;
      break;

    case git::Index::Staged:
      state = Qt::Checked;
      break;

    case git::Index::Conflicted:
      disabled = (mPatch.count() > 0);
      break;
  }

  mCheck->setCheckState(state);
  mCheck->setEnabled(!disabled);
}

//###############################################################################
//###############      FileWidget ###########################################
//###############################################################################

FileWidget::FileWidget(DiffView *view, const git::Diff &diff, git::Patch &patch,
                       const git::Patch &staged, const QModelIndex modelIndex,
                       const QString &name, const QString &path, bool submodule,
                       QWidget *parent)
    : QWidget(parent), mView(view), mDiff(diff), mPatch(patch),
      mModelIndex(modelIndex) {
  auto stageState = static_cast<git::Index::StagedState>(
      mModelIndex.data(Qt::CheckStateRole).toInt());
  setObjectName("FileWidget");
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  bool binary = patch.isBinary();
  if (patch.isUntracked()) {
    QFile dev(path);
    if (dev.open(QFile::ReadOnly)) {
      QByteArray content = dev.readAll();
      git::Buffer buffer(content.constData(), content.length());
      binary = buffer.isBinary();
    }
  }

  bool lfs = patch.isLfsPointer();
  mHeader =
      new _FileWidget::Header(diff, patch, binary, lfs, submodule, parent);
  mHeader->setStageState(stageState);
  connect(mHeader, &_FileWidget::Header::stageStateChanged, this,
          &FileWidget::headerCheckStateChanged);
  connect(mHeader, &_FileWidget::Header::discard, this, &FileWidget::discard);
  layout->addWidget(mHeader);

  DisclosureButton *disclosureButton = mHeader->disclosureButton();
  if (disclosure)
    connect(disclosureButton, &DisclosureButton::toggled, [this](bool visible) {
      if (mHeader->lfsButton() && !visible) {
        mHunks.first()->setVisible(false);
        if (!mImages.isEmpty())
          mImages.first()->setVisible(false);
        return;
      }

      if (mHeader->lfsButton() && visible) {
        bool checked = mHeader->lfsButton()->isChecked();
        mHunks.first()->setVisible(!checked);
        if (!mImages.isEmpty())
          mImages.first()->setVisible(checked);
        return;
      }

      foreach (HunkWidget *hunk, mHunks)
        hunk->setVisible(visible);
    });

  if (diff.isStatusDiff()) {
    // Collapse on check.
    if (disclosure)
      connect(mHeader->check(), &QCheckBox::stateChanged, [this](int state) {
        mHeader->disclosureButton()->setChecked(state != Qt::Checked);
      });
  }

  // Try to load an image from the file.
  if (binary) {
    layout->addWidget(addImage(disclosureButton, mPatch));
    return;
  }

  mHunkLayout = new QVBoxLayout();
  layout->addLayout(mHunkLayout);
  layout->addSpacerItem(new QSpacerItem(
      0, 0, QSizePolicy::Expanding,
      QSizePolicy::Expanding)); // so the hunkwidget is always starting from top
                                // and is not distributed over the hole
                                // filewidget

  updatePatch(patch, staged, name, path, submodule);

  // LFS
  if (QToolButton *lfsButton = mHeader->lfsButton()) {
    connect(lfsButton, &QToolButton::clicked,
            [this, layout, disclosureButton, lfsButton](bool checked) {
              lfsButton->setText(checked ? tr("Show Pointer")
                                         : tr("Show Object"));
              mHunks.first()->setVisible(!checked);

              // Image already loaded.
              if (!mImages.isEmpty()) {
                mImages.first()->setVisible(checked);
                return;
              }

              // Load image.
              layout->addWidget(addImage(disclosureButton, mPatch, true));
            });
  }

  // Start hidden when the file is checked.
  bool expand = (mHeader->check()->checkState() == Qt::Unchecked);

  if (Settings::instance()
              ->value(Setting::Id::AutoCollapseAddedFiles)
              .toBool() == true &&
      patch.status() == GIT_DELTA_ADDED)
    expand = false;

  if (Settings::instance()
              ->value(Setting::Id::AutoCollapseDeletedFiles)
              .toBool() == true &&
      patch.status() == GIT_DELTA_DELETED)
    expand = false;

  disclosureButton->setChecked(expand);

  // Handle conflict resolution.
  if (QToolButton *save = mHeader->saveButton()) {
    connect(save, &QToolButton::clicked, [this] {
      git::Repository repo = mPatch.repo();

      auto resolution = mHeader->resolution();
      if (resolution == git::Patch::ConflictResolution::Unresolved) {
        return;
      }

      auto conflict = repo.index().conflict(mPatch.name());
      auto id = resolution == git::Patch::ConflictResolution::Ours
                    ? conflict.ours
                    : conflict.theirs;

      if (id.isNull()) {
        QFile::remove(repo.workdir().filePath(mPatch.name()));
        repo.index().setStaged(QStringList{mPatch.name()}, true);
        return;
      }

      if (!id.isValid()) {
        return;
      }

      auto blob = repo.lookupBlob(id);
      if (!blob.isValid()) {
        return;
      }

      // Write file to disk.
      QSaveFile file(repo.workdir().filePath(mPatch.name()));
      if (!file.open(QFile::WriteOnly))
        return;

      file.write(blob.content());
      file.commit();

      repo.index().setStaged(QStringList{mPatch.name()}, true);

      RepoView::parentView(this)->refresh();
    });
  }
}

void FileWidget::updateHunks(git::Patch stagedPatch) {
  for (auto hunk : mHunks)
    hunk->load(stagedPatch, true);
}

bool FileWidget::isEmpty() { return (mHunks.isEmpty() && mImages.isEmpty()); }

void FileWidget::setStageState(git::Index::StagedState state) {
  mHeader->setStageState(state);

  for (auto hunk : mHunks)
    hunk->setStageState(state);
}

QModelIndex FileWidget::modelIndex() { return mModelIndex; }

QToolButton *_FileWidget::Header::saveButton() const { return mSave; }

QToolButton *_FileWidget::Header::undoButton() const { return mUndo; }

QToolButton *_FileWidget::Header::oursButton() const { return mOurs; }

QToolButton *_FileWidget::Header::theirsButton() const { return mTheirs; }

git::Patch::ConflictResolution _FileWidget::Header::resolution() const {
  return mResolution;
}

void FileWidget::updatePatch(git::Patch &patch, const git::Patch &staged,
                             const QString &name, const QString &path,
                             bool submodule) {
  mHeader->updatePatch(patch);

  bool lfs = patch.isLfsPointer();

  // remove all hunks
  QLayoutItem *child;
  while ((child = mHunkLayout->takeAt(0)) != 0) {
    delete child;
  }
  // Add untracked file content.
  if (patch.isUntracked()) {
    if (!QFileInfo(path).isDir())
      mHunkLayout->addWidget(addHunk(mDiff, patch, staged, -1, lfs, submodule));
    return;
  }

  // Generate a diff between the head tree and index.
  QSet<int> stagedHunks;
  if (staged.isValid()) {
    for (int i = 0; i < staged.count(); ++i)
      stagedHunks.insert(staged.lineNumber(i, 0, git::Diff::OldFile));
  }

  // Add diff hunks.
  int hunkCount = patch.count();
  for (int hidx = 0; hidx < hunkCount; ++hidx) {
    HunkWidget *hunk = addHunk(mDiff, patch, staged, hidx, lfs, submodule);
    int startLine = patch.lineNumber(hidx, 0, git::Diff::OldFile);
    // hunk->header()->check()->setChecked(stagedHunks.contains(startLine)); //
    // not correct, because it could also only a part of the hunk staged (single
    // lines)
    mHunkLayout->addWidget(hunk);
  }
}

_FileWidget::Header *FileWidget::header() const { return mHeader; }

QString FileWidget::name() const { return mPatch.name(); }

QList<HunkWidget *> FileWidget::hunks() const { return mHunks; }

QWidget *FileWidget::addImage(DisclosureButton *button, const git::Patch patch,
                              bool lfs) {
  Images *images = new Images(patch, lfs, this);

  // Hide on file collapse.
  if (!lfs && disclosure)
    connect(button, &DisclosureButton::toggled, images, &QLabel::setVisible);

  // Remember image.
  mImages.append(images);

  return images;
}

HunkWidget *FileWidget::addHunk(const git::Diff &diff, git::Patch &patch,
                                const git::Patch &staged, int index, bool lfs,
                                bool submodule) {
  HunkWidget *hunk =
      new HunkWidget(mView, diff, patch, staged, index, lfs, submodule, this);

  connect(hunk, &HunkWidget::stageStateChanged,
          [this, hunk](git::Index::StagedState state) {
            this->stageHunks(hunk, state, false);
          });
  connect(hunk, &HunkWidget::discardSignal, this, &FileWidget::discardHunk);
  TextEditor *editor = hunk->editor(false);

  // Respond to editor diagnostic signal.
  connect(editor, &TextEditor::diagnosticAdded,
          [this](int line, const TextEditor::Diagnostic &diag) {
            emit diagnosticAdded(diag.kind);
          });

  // Remember hunk.
  mHunks.append(hunk);

  return hunk;
}

void FileWidget::stageHunks(const HunkWidget *hunk,
                            git::Index::StagedState stageState,
                            bool completeFile, bool completeFileStaged) {
  if (mSupressStaging)
    return;

  git::Index index = mDiff.index();
  if (!index.isValid()) // why the index can be invalid?
    return;

  int staged = 0;
  int unstaged = 0;
  for (int i = 0; i < mHunks.size(); ++i) {
    git::Index::StagedState state;
    if (mHunks[i] == hunk)
      state =
          stageState; // because the current hunk did not change the state yet.
    else
      state = mHunks[i]->stageState();
    if (state == git::Index::Staged)
      staged++;
    else if (state == git::Index::Unstaged)
      unstaged++;
  }

  mSuppressUpdate = true;

  if ((staged == mHunks.size() && mHunks.size() > 0) ||
      (completeFile && completeFileStaged)) {
    // if the file does not contain hunks, it should be always staged!
    emit stageStateChanged(mModelIndex, git::Index::Staged);
    mSuppressUpdate = false;
    return;
  } else if (completeFile && !completeFileStaged) {
    emit stageStateChanged(mModelIndex, git::Index::Unstaged);
    mSuppressUpdate = false;
    return;
  }

  if (unstaged == mHunks.size()) {
    emit stageStateChanged(mModelIndex, git::Index::Unstaged);
    mSuppressUpdate = false;
    return;
  }

  // when changing a line in a file,
  // two lines are visible, the old one and the new one.
  // When staging only one line, for example the added line,
  // then the unstaged (removed line) and the added line shall be
  // available in the file.
  QString name = mPatch.name();
  git::Repository repo = mPatch.repo();
  git::Blob blob = repo.lookupBlob(repo.workdirId(name));

  QByteArray fileContent = blob.content();

  QByteArray buffer;
  QList<QList<QByteArray>> image;
  git::Patch::populatePreimage(image, fileContent);
  for (int i = 0; i < mHunks.size(); ++i) {
    QByteArray hunk_content;
    hunk_content = mHunks[i]->apply();
    mPatch.apply(image, i, hunk_content);
  }
  buffer = mPatch.generateResult(image);

  // Add the buffer to the index.
  index.add(mPatch.name(), buffer);
  mSuppressUpdate = false;

  // TODO: index.add should notify the model directly!
  emit stageStateChanged(mModelIndex, git::Index::PartiallyStaged);
}

void FileWidget::discardHunk() {
  HunkWidget *hunk = static_cast<HunkWidget *>(QObject::sender());
  git::Repository repo = mPatch.repo();
  if (mPatch.isUntracked()) {
    repo.workdir().remove(mPatch.name());
    return;
  }

  QString name = mPatch.name();
  git::Blob blob = repo.lookupBlob(repo.workdirId(name));
  QByteArray fileContent = blob.content();

  QByteArray buffer;
  for (int i = 0; i < mHunks.size(); ++i) {
    QByteArray hunk_content;
    if (mHunks[i] == hunk) {
      hunk_content = mHunks[i]->hunk();
      buffer = mPatch.apply(i, hunk_content, fileContent);
    }
  }

  QSaveFile file(repo.workdir().filePath(name));
  if (!file.open(QFile::WriteOnly))
    return;

  file.write(buffer);
  if (!file.commit())
    return;

  // FIXME: Work dir changed?
  RepoView::parentView(this)->refresh();
}

void FileWidget::discard() {
  QString name = mPatch.name();
  bool untracked = mPatch.isUntracked();
  QString path = mPatch.repo().workdir().filePath(name);
  QString arg = QFileInfo(path).isDir() ? FileWidget::tr("Directory")
                                        : FileWidget::tr("File");
  QString title = untracked ? FileWidget::tr("Remove %1?").arg(arg)
                            : FileWidget::tr("Discard Changes?");
  QString text =
      untracked ? FileWidget::tr("Are you sure you want to remove '%1'?")
                : FileWidget::tr(
                      "Are you sure you want to discard all changes in '%1'?");
  QMessageBox *dialog = new QMessageBox(
      QMessageBox::Warning, title, text.arg(name), QMessageBox::Cancel, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setInformativeText(FileWidget::tr("This action cannot be undone."));

  QString button = untracked ? FileWidget::tr("Remove %1").arg(arg)
                             : FileWidget::tr("Discard Changes");
  QPushButton *discard = dialog->addButton(button, QMessageBox::AcceptRole);
  connect(discard, &QPushButton::clicked,
          [this, untracked] { emit discarded(mModelIndex); });

  dialog->exec();
}

void FileWidget::headerCheckStateChanged(int state) {
  assert(state != Qt::PartiallyChecked); // makes no sense, that the user can
                                         // select partially selected

  if (state == Qt::Checked)
    emit stageStateChanged(mModelIndex, git::Index::Staged);
  else
    emit stageStateChanged(mModelIndex, git::Index::Unstaged);
}
