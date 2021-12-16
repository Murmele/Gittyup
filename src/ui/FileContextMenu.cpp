//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "FileContextMenu.h"
#include "RepoView.h"
#include "IgnoreDialog.h"
#include "conf/Settings.h"
#include "dialogs/SettingsDialog.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "tools/EditTool.h"
#include "tools/ShowTool.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDesktopServices>

namespace {

void warnRevisionNotFound(
  QWidget *parent,
  const QString &fragment,
  const QString &file)
{
  QString title = FileContextMenu::tr("Revision Not Found");
  QString text = FileContextMenu::tr(
    "The selected file doesn't have a %1 revision.").arg(fragment);
  QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok, parent);
  msg.setInformativeText(file);
  msg.exec();
}

} // anon. namespace

FileContextMenu::FileContextMenu(
  RepoView *view,
  const QStringList &files,
  const git::Index &index,
  QWidget *parent)
  : QMenu(parent),
    mFiles(files),
    mView(view)
{
  // Show diff and merge tools for the currently selected diff.
  git::Diff diff = view->diff();
  git::Repository repo = view->repo();

  // Create external tools.
  QList<ExternalTool *> showTools;
  QList<ExternalTool *> editTools;
  QList<ExternalTool *> diffTools;
  QList<ExternalTool *> mergeTools;
  foreach (const QString &file, files) {
    // Convert to absolute path.
    QString path = repo.workdir().filePath(file);

    // Add show tool.
    showTools.append(new ShowTool(path, this));

    // Add edit tool.
    editTools.append(new EditTool(path, this));

    // Add diff or merge tool.
    if (ExternalTool *tool = ExternalTool::create(file, diff, repo, this)) {
      switch (tool->kind()) {
        case ExternalTool::Diff:
          diffTools.append(tool);
          break;

        case ExternalTool::Merge:
          mergeTools.append(tool);
          break;

        default:
          Q_ASSERT(false);
          break;
      }

      connect(tool, &ExternalTool::error, [this](ExternalTool::Error error) {
        if (error != ExternalTool::BashNotFound)
          return;

        QString title = tr("Bash Not Found");
        QString text = tr("Bash was not found on your PATH.");
        QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok, this);
        msg.setInformativeText(tr("Bash is required to execute external tools."));
        msg.exec();
      });
    }
  }

  // Add external tool actions.
  addExternalToolsAction(showTools);
  addExternalToolsAction(editTools);
  addExternalToolsAction(diffTools);
  addExternalToolsAction(mergeTools);

  if (!isEmpty())
    addSeparator();

  QList<git::Commit> commits = view->commits();
  if (commits.isEmpty()) {
    if (index.isValid()) {
      // Stage/Unstage
      QAction *stage = addAction(tr("Stage"), [index, files] {
        git::Index(index).setStaged(files, true);
      });

      QAction *unstage = addAction(tr("Unstage"), [index, files] {
        git::Index(index).setStaged(files, false);
      });

      int staged = 0;
      int unstaged = 0;
      foreach (const QString &file, files) {
        switch (index.isStaged(file)) {
          case git::Index::Disabled:
            break;

          case git::Index::Unstaged:
            ++unstaged;
            break;

          case git::Index::PartiallyStaged:
            ++staged;
            ++unstaged;
            break;

          case git::Index::Staged:
            ++staged;
            break;

          case git::Index::Conflicted:
            // FIXME: Resolve conflicts?
            break;
        }
      }

      stage->setEnabled(unstaged > 0);
      unstage->setEnabled(staged > 0);

      addSeparator();
    }

    // Discard
    QStringList modified;
    QStringList untracked;
    if (diff.isValid()) {
      foreach (const QString &file, files) {
        int index = diff.indexOf(file);
        if (index < 0)
          continue;

        switch (diff.status(index)) {
          case GIT_DELTA_DELETED:
          case GIT_DELTA_MODIFIED:
            modified.append(file);
            break;

          case GIT_DELTA_UNTRACKED:
            untracked.append(file);
            break;

          default:
            break;
        }
      }
    }

    QAction *discard = addAction(tr("Discard Changes"), [view, modified] {
      QMessageBox *dialog = new QMessageBox(
        QMessageBox::Warning, tr("Discard Changes?"),
        tr("Are you sure you want to discard changes in the selected files?"),
        QMessageBox::Cancel, view);
      dialog->setAttribute(Qt::WA_DeleteOnClose);
      dialog->setInformativeText(tr("This action cannot be undone."));
      dialog->setDetailedText(modified.join('\n'));

      QString text = tr("Discard Changes");
      QPushButton *discard = dialog->addButton(text, QMessageBox::AcceptRole);
      connect(discard, &QPushButton::clicked, [view, modified] {
        git::Repository repo = view->repo();
        int strategy = GIT_CHECKOUT_FORCE;
        if (!repo.checkout(git::Commit(), nullptr, modified, strategy)) {
          QString text = tr("%1 files").arg(modified.size());
          LogEntry *parent = view->addLogEntry(text, tr("Discard"));
          view->error(parent, tr("discard"), text);
        }

        // FIXME: Work dir changed?
        view->refresh();
      });

      dialog->open();
    });

    QAction *remove = addAction(tr("Remove Untracked Files"), [view, untracked] {
      view->clean(untracked);
    });

    discard->setEnabled(!modified.isEmpty());
    remove->setEnabled(!untracked.isEmpty());

    // Ignore
    QAction *ignore = addAction(tr("Ignore"));
    connect(ignore, &QAction::triggered, this, &FileContextMenu::ignoreFile);

    if (!diff.isValid()) {
      ignore->setEnabled(false);
    } else {
      foreach (const QString &file, files) {
        int index = diff.indexOf(file);
        if (index < 0)
          continue;

        if (diff.status(index) != GIT_DELTA_UNTRACKED) {
          ignore->setEnabled(false);
          break;
        }
      }
    }

  } else {
    // Checkout
    QAction *checkout = addAction(tr("Checkout"), [this, view, files] {
      view->checkout(view->commits().first(), files);
      view->setViewMode(RepoView::DoubleTree);
    });

    // Checkout to ...
    QAction *checkoutTo = addAction(tr("Checkout to ..."), [this, view, files] {
      QFileDialog d(this);
      d.setFileMode(QFileDialog::FileMode::Directory);
      d.setOption(QFileDialog::ShowDirsOnly);
      d.setWindowTitle(tr("Select checkout directory"));
      if (d.exec()) {
          auto folder = d.selectedFiles().first();
          view->checkout(view->commits().first(), files, folder);
          view->setViewMode(RepoView::DoubleTree);
      }
      });


    QAction *openWith = addAction(tr("Open with ..."), [this, view, files] {
      QString folder = ".";
      view->checkout(view->commits().first(), QStringList(files.first()), folder);
      view->setViewMode(RepoView::DoubleTree);
      QDesktopServices::openUrl(QUrl::fromLocalFile(QDir(folder).filePath(files.first())));
    });

    auto isBare = view->repo().isBare();
    checkout->setEnabled(!isBare);
    checkoutTo->setEnabled(!isBare);
    openWith->setEnabled(!isBare);

    git::Commit commit = commits.first();
    foreach (const QString &file, files) {
      if (commit.tree().id(file) == repo.workdirId(file)) {
        checkout->setEnabled(false);
        checkoutTo->setEnabled(false);
        openWith->setEnabled(false);
        break;
      }
    }
  }



  // LFS
  if (repo.lfsIsInitialized()) {
    addSeparator();

    bool locked = false;
    foreach (const QString &file, files) {
      if (repo.lfsIsLocked(file)) {
        locked = true;
        break;
      }
    }

    addAction(locked ? tr("Unlock") : tr("Lock"), [view, files, locked] {
      view->lfsSetLocked(files, !locked);
    });
  }

  // Add single selection actions.
  if (files.size() == 1) {
    addSeparator();

    // Copy File Name
    QDir dir = repo.workdir();
    QString file = files.first();
    QString rel = QDir::toNativeSeparators(file);
    QString abs = QDir::toNativeSeparators(dir.filePath(file));
    QString name = QFileInfo(file).fileName();
    QMenu *copy = addMenu(tr("Copy File Name"));
    if (!name.isEmpty() && name != file) {
      copy->addAction(name, [name] {
        QApplication::clipboard()->setText(name);
      });
    }
    copy->addAction(rel, [rel] {
      QApplication::clipboard()->setText(rel);
    });
    copy->addAction(abs, [abs] {
      QApplication::clipboard()->setText(abs);
    });

    addSeparator();

    // History
    addAction(tr("Filter History"), [view, file] {
      view->setPathspec(file);
    });

    // Navigate
    QMenu *navigate = addMenu(tr("Navigate to"));
    QAction *nextAct = navigate->addAction(tr("Next Revision"));
    connect(nextAct, &QAction::triggered, [view, file] {
      if (git::Commit next = view->nextRevision(file)) {
        view->selectCommit(next, file);
      } else {
        warnRevisionNotFound(view, tr("next"), file);
      }
    });

    QAction *prevAct = navigate->addAction(tr("Previous Revision"));
    connect(prevAct, &QAction::triggered, [view, file] {
      if (git::Commit prev = view->previousRevision(file)) {
        view->selectCommit(prev, file);
      } else {
        warnRevisionNotFound(view, tr("previous"), file);
      }
    });

    if (index.isValid() && index.isStaged(file)) {
      addSeparator();

      // Executable
      git_filemode_t mode = index.mode(file);
      bool exe = (mode == GIT_FILEMODE_BLOB_EXECUTABLE);
      QString exeName = exe ? tr("Unset Executable") : tr("Set Executable");
      QAction *exeAct = addAction(exeName, [index, file, exe] {
        git::Index(index).setMode(
          file, exe ? GIT_FILEMODE_BLOB : GIT_FILEMODE_BLOB_EXECUTABLE);
      });

      exeAct->setEnabled(exe || mode == GIT_FILEMODE_BLOB);
    }
  }
}

void FileContextMenu::ignoreFile()
{
  QString ignore;

  if (!mFiles.count())
      return;

  for (int i=0; i < mFiles.count() - 1; i++) {
     ignore.append(mFiles[i] + "\n");
  }
  ignore.append(mFiles.last());

 IgnoreDialog d(ignore, this);
 if (d.exec()) {
    if (!ignore.isEmpty())
        mView->ignore(ignore);
 }
}

void FileContextMenu::addExternalToolsAction(
  const QList<ExternalTool *> &tools)
{
  if (tools.isEmpty())
    return;

  // Add action.
  QAction *action = addAction(tools.first()->name(), [this, tools] {
    foreach (ExternalTool *tool, tools) {
      if (tool->start())
        return;

      QString kind;
      switch (tool->kind()) {
        case ExternalTool::Show:
          return;

        case ExternalTool::Edit:
          kind = tr("edit");
          break;

        case ExternalTool::Diff:
          kind = tr("diff");
          break;

        case ExternalTool::Merge:
          kind = tr("merge");
          break;
      }

      QString title = tr("External Tool Not Found");
      QString text = tr("Failed to execute external %1 tool.");
      QMessageBox::warning(this, title, text.arg(kind), QMessageBox::Ok);
      SettingsDialog::openSharedInstance(SettingsDialog::Tools);
    }
  });

  // Disable if any tools are invalid.
  foreach (ExternalTool *tool, tools) {
    if (!tool->isValid()) {
      action->setEnabled(false);
      break;
    }
  }
}
