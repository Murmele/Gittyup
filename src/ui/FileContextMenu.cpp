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
#include "Debug.h"
#include "dialogs/SettingsDialog.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "host/Repository.h"
#include "tools/EditTool.h"
#include "tools/ShowTool.h"
#include "tools/DiffTool.h"
#include "tools/MergeTool.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDesktopServices>
#include <QFileInfo>
#include <qfileinfo.h>

namespace {

void warnRevisionNotFound(QWidget *parent, const QString &fragment,
                          const QString &file) {
  QString title = FileContextMenu::tr("Revision Not Found");
  QString text =
      FileContextMenu::tr("The selected file doesn't have a %1 revision.")
          .arg(fragment);
  QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok, parent);
  msg.setInformativeText(file);
  msg.exec();
}

void handlePath(const git::Repository &repo, const QString &path,
                const git::Diff &diff, QStringList &modified,
                QStringList &untracked) {
  auto fullPath = repo.workdir().absoluteFilePath(path);
  Debug("FileContextMenu handlePath()" << path);

  if (QFileInfo(fullPath).isDir()) {
    auto dir = QDir(path);

    for (auto entry : QDir(fullPath).entryList(
             QDir::NoDotAndDotDot | QDir::Hidden | QDir::Dirs | QDir::Files)) {
      handlePath(repo, dir.filePath(entry), diff, modified, untracked);
    }

  } else {
    int index = diff.indexOf(path);
    if (index < 0)
      return;

    switch (diff.status(index)) {
      case GIT_DELTA_DELETED:
      case GIT_DELTA_MODIFIED:
        modified.append(path);
        break;

      case GIT_DELTA_UNTRACKED:
        untracked.append(path);
        break;

      default:
        break;
    }
  }
}

} // namespace

FileContextMenu::FileContextMenu(RepoView *view, const QStringList &files,
                                 const git::Index &index, QWidget *parent)
    : QMenu(parent), mView(view), mFiles(files) {
  git::Diff diff = view->diff();
  if (!diff.isValid())
    return;

  auto errFunc = [this](ExternalTool::Error error) {
    if (error != ExternalTool::BashNotFound)
      return;

    QString title = tr("Bash Not Found");
    QString text = tr("Bash was not found on your PATH.");
    QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok,
                    this);
    msg.setInformativeText(
        tr("Bash is required to execute external tools."));
    msg.exec();
  };

  git::Repository repo = view->repo();

  // Create external tools.
  QList<ExternalTool *> showTools, editTools, diffTools, mergeTools;
  attachTool(new ShowTool(files, diff, repo, this), showTools);
  attachTool(new EditTool(files, diff, repo, this), editTools);
  if (diff.isConflicted()) {
    attachTool(new MergeTool(files, diff, repo, this), mergeTools);
  } 
  else {
    attachTool(new DiffTool(files, diff, repo, this), diffTools);
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
    handleUncommittedChanges(index, files);
  } else {
    handleCommits(commits, files);
  }

  // TODO: moving this into handleWorkingDirChanges()? Because
  // Locking committed files does not make sense or?
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

    addAction(locked ? tr("Unlock") : tr("Lock"),
              [view, files, locked] { view->lfsSetLocked(files, !locked); });
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
      copy->addAction(name,
                      [name] { QApplication::clipboard()->setText(name); });
    }
    copy->addAction(rel, [rel] { QApplication::clipboard()->setText(rel); });
    copy->addAction(abs, [abs] { QApplication::clipboard()->setText(abs); });

    addSeparator();

    // History
    addAction(tr("Filter History"), [view, file] { view->setPathspec(file); });

    // Navigate
    QMenu *navigate = addMenu(tr("Navigate to"));
    QAction *nextAct = navigate->addAction(tr("Next Revision"));
    QMenu::connect(nextAct, &QAction::triggered, [view, file] {
      if (git::Commit next = view->nextRevision(file)) {
        view->selectCommit(next, file);
      } else {
        warnRevisionNotFound(view, tr("next"), file);
      }
    });

    QAction *prevAct = navigate->addAction(tr("Previous Revision"));
    QMenu::connect(prevAct, &QAction::triggered, [view, file] {
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
        git::Index(index).setMode(file, exe ? GIT_FILEMODE_BLOB
                                            : GIT_FILEMODE_BLOB_EXECUTABLE);
      });

      exeAct->setEnabled(exe || mode == GIT_FILEMODE_BLOB);
    }
  }
}

void FileContextMenu::handleUncommittedChanges(const git::Index &index,
                                               const QStringList &files) {
  git::Diff diff = mView->diff();
  git::Repository repo = mView->repo();
  const auto view = mView;
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
  // copied from DiffTreeModel.cpp
  // handle submodules
  auto s = repo.submodules();
  QList<git::Submodule> submodules;
  QStringList filePatches;
  for (auto trackedPatch : files) {
    bool is_submodule = false;
    for (auto submodule : s) {
      if (submodule.path() == trackedPatch) {
        is_submodule = true;
        submodules.append(submodule);
        break;
      }
    }
    if (!is_submodule)
      filePatches.append(trackedPatch);
  }

  // handle files not submodules
  foreach (const QString &file, filePatches) {
    handlePath(repo, file, diff, modified, untracked);
  }

  QAction *discard =
      addAction(tr("Discard Changes"), [view, modified, submodules] {
        QMessageBox *dialog =
            new QMessageBox(QMessageBox::Warning, tr("Discard Changes?"),
                            tr("Are you sure you want to discard changes in "
                               "the selected files?"),
                            QMessageBox::Cancel, view);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setInformativeText(tr("This action cannot be undone."));
        QString detailedText = modified.join('\n');
        for (const auto &s : submodules)
          detailedText += s.path() + " " + tr("(Submodule)") + "\n";
        dialog->setDetailedText(detailedText);

        // Expand the Show Details
        foreach (QAbstractButton *button, dialog->buttons()) {
          if (dialog->buttonRole(button) == QMessageBox::ActionRole) {
            button->click(); // click it to expand the text
            break;
          }
        }

        QString text = tr("Discard Changes");
        QPushButton *discard = dialog->addButton(text, QMessageBox::AcceptRole);
        discard->setObjectName("DiscardButton");
        QMenu::connect(discard, &QPushButton::clicked, [view, modified, submodules] {
          git::Repository repo = view->repo();
          int strategy = GIT_CHECKOUT_FORCE;
          if (modified.count() &&
              !repo.checkout(git::Commit(), nullptr, modified, strategy)) {
            QString text = tr("%1 files").arg(modified.size());
            LogEntry *parent = view->addLogEntry(text, tr("Discard"));
            view->error(parent, tr("discard"), text);
          }
          view->updateSubmodules(submodules, true, false, true);

          if (submodules.isEmpty())
            view->refresh();
        });

        dialog->open();
      });
  discard->setEnabled(!modified.isEmpty() || submodules.count());

  QAction *remove = addAction(tr("Remove Untracked Files"),
                              [view, untracked] { view->clean(untracked); });
  remove->setObjectName("RemoveAction");
  remove->setEnabled(!untracked.isEmpty());

  // Ignore
  QAction *ignore = addAction(tr("Ignore"));
  ignore->setObjectName("IgnoreAction");
  QMenu::connect(ignore, &QAction::triggered, this, &FileContextMenu::ignoreFile);
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

void FileContextMenu::handleCommits(const QList<git::Commit> &commits,
                                    const QStringList &files) {
  // because this might not live anymore
  // when the lambdas are handled
  const auto view = mView;
  git::Repository repo = view->repo();

  // Checkout
  QAction *checkout = addAction(tr("Checkout"), [view, files] {
    view->checkout(view->commits().first(), files);
    view->setViewMode(RepoView::DoubleTree);
  });

  // Checkout to ...
  QAction *checkoutTo =
      addAction(tr("Save Selected Version as ..."), [this, view, files] {
        QFileDialog d(this); // TODO: this might not live anymore??
        d.setFileMode(QFileDialog::FileMode::Directory);
        d.setOption(QFileDialog::ShowDirsOnly);
        d.setWindowTitle(tr("Select new file directory"));
        if (d.exec()) {
          const auto folder = d.selectedFiles().first();
          const auto save =
              view->addLogEntry(tr("Saving files"),
                                tr("Saving files of selected version to disk"));
          for (const auto &file : files) {
            const auto saveFile =
                view->addLogEntry(tr("Save file ") + file, "Save file", save);
            // assumption. file is a file not a folder!
            if (!exportFile(view, folder, file))
              view->error(saveFile, "save file", file, tr("Invalid Blob"));
          }
          view->setViewMode(RepoView::DoubleTree);
        }
        return true;
      });

  QAction *open = addAction(tr("Open this version"), [this, view, files] {
    QString folder = QDir::tempPath();
    const auto &file = files.first();
    auto filename = file.split("/").last();

    auto logentry =
        view->addLogEntry(tr("Opening file"), tr("Open ") + filename);

    if (exportFile(view, folder, file))
      QDesktopServices::openUrl(QUrl::fromLocalFile(
          QFileInfo(folder + "/" + filename).absoluteFilePath()));
    else
      view->error(logentry, tr("open file"), filename, tr("Blob is invalid."));
  });

  // should show a dialog to select an application
  // Don't forgett to uncomment "openWith->setEnabled(!isBare);" below
  //	QAction *openWith = addAction(tr("Open this Version with ..."),
  //[this,
  // view, files] { 	  QString folder = QDir::tempPath(); const auto&
  // file = files.first(); 	  auto filename = file.split("/").last();

  //	  auto logentry = view->addLogEntry(tr("Opening file with ..."),
  // tr("Open ") + filename);

  //	  if (exportFile(view, folder, file))
  //		QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(folder
  //+
  //"/"
  //+ filename).absoluteFilePath())); 	  else
  // view->error(logentry, tr("open file"), filename, tr("Blob is
  // invalid."));
  //	});

  auto isBare = view->repo().isBare();
  const auto blob = view->commits().first().blob(files.first());
  checkout->setEnabled(!isBare);
  checkout->setToolTip(!isBare ? ""
                               : tr("Unable to checkout bare repositories"));
  checkoutTo->setEnabled(!isBare && blob.isValid());
  checkoutTo->setToolTip(!isBare ? ""
                                 : tr("Unable to checkout bare repositories"));
  open->setEnabled(!isBare && blob.isValid());
  open->setToolTip(!isBare ? ""
                           : tr("Unable to open files from bare repository"));
  // openWith->setEnabled(!isBare && blob.isValid());

  /* disable checkout if the file is already
   * in the current working directory */
  git::Commit commit = commits.first();
  foreach (const QString &file, files) {
    if (commit.tree().id(file) == repo.workdirId(file)) {
      checkout->setEnabled(false);
      checkout->setToolTip(
          tr("The file is already in the current working directory"));
      break;
    }
  }
}

void FileContextMenu::ignoreFile() {
  if (!mFiles.count())
    return;

  auto d = new IgnoreDialog(mFiles.join('\n'), parentWidget());
  d->setAttribute(Qt::WA_DeleteOnClose);

  auto *view = mView;
  QMenu::connect(d, &QDialog::accepted, [d, view]() {
    auto ignore = d->ignoreText();
    if (!ignore.isEmpty())
      view->ignore(ignore);
  });

  d->open();
}

bool FileContextMenu::exportFile(const RepoView *view, const QString &folder,
                                 const QString &file) {
  const auto blob = view->commits().first().blob(file);
  if (!blob.isValid())
    return false;

  auto filename = file.split("/").last();
  QFile f(folder + "/" + filename);
  if (!f.open(QFile::ReadWrite))
    return false;

  f.write(blob.content());
  f.close();
  return true;
}

void FileContextMenu::addExternalToolsAction(
    const QList<ExternalTool *> &tools) {
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

void FileContextMenu::attachTool(ExternalTool *tool, 
                                 QList<ExternalTool *> &tools)
{
  tools.append(tool);

  QMenu::connect(tool, &ExternalTool::error, [this](ExternalTool::Error error) {
    if (error != ExternalTool::BashNotFound)
      return;

    QString title = tr("Bash Not Found");
    QString text = tr("Bash was not found on your PATH.");
    QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok,
                    this);
    msg.setInformativeText(
        tr("Bash is required to execute external tools."));
    msg.exec();
  });
}
