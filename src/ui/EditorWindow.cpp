//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "EditorWindow.h"
#include "BlameEditor.h"
#include "MenuBar.h"
#include "editor/TextEditor.h"
#include "git/Reference.h"
#include <QMessageBox>

EditorWindow::EditorWindow(const git::Repository &repo, QWidget *parent)
    : QMainWindow(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  resize(800, 800);

  BlameEditor *widget = new BlameEditor(repo, this);
  connect(widget, &BlameEditor::saved, [this, repo] {
    updateWindowTitle();
    if (!repo.isValid())
      return;

    // Notify window that the head branch is changed.
    emit repo.notifier()->referenceUpdated(repo.head());
  });

  TextEditor *editor = widget->editor();
  connect(editor, &TextEditor::savePointChanged, this,
          &QWidget::setWindowModified);

  // Connect menu bar actions.
  if (MenuBar *menuBar = MenuBar::instance(this)) {
    connect(editor, &TextEditor::savePointChanged, menuBar,
            &MenuBar::updateSave);
    connect(editor, &TextEditor::updateUi, menuBar, &MenuBar::updateUndoRedo);
    connect(editor, &TextEditor::updateUi, menuBar,
            &MenuBar::updateCutCopyPaste);
  }

  setCentralWidget(widget);
}

BlameEditor *EditorWindow::widget() const {
  return static_cast<BlameEditor *>(centralWidget());
}

void EditorWindow::updateWindowTitle() {
  BlameEditor *editor = widget();
  setWindowTitle(QString("%1: %2[*]").arg(editor->name(), editor->revision()));
}

EditorWindow *EditorWindow::open(const QString &path, const git::Blob &blob,
                                 const git::Commit &commit,
                                 const git::Repository &repo) {
  QDir dir = repo.isValid() ? repo.workdir() : QDir::current();
  QFileInfo file(QDir::isAbsolutePath(path) ? path : dir.filePath(path));
  if (!file.exists() || !file.isFile())
    return nullptr;

  EditorWindow *window = new EditorWindow(repo);
  BlameEditor *widget = window->widget();

  // Try to load the content.
  if (!widget->load(path, blob, commit)) {
    delete window;
    return nullptr;
  }

  // Show the window.
  window->show();
  return window;
}

void EditorWindow::showEvent(QShowEvent *event) {
  updateWindowTitle();
  QMainWindow::showEvent(event);
}

void EditorWindow::closeEvent(QCloseEvent *event) {
  // Prompt to save.
  BlameEditor *editor = widget();
  if (editor->editor()->isModified()) {
    QString text =
        tr("'%1' has been modified. Do you want to save your changes?");
    QMessageBox::StandardButton button = QMessageBox::warning(
        this, tr("Save Changes?"), text.arg(editor->name()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (button) {
      case QMessageBox::Cancel:
        event->ignore();
        return;
      case QMessageBox::Save:
        editor->save();
        break;
      default:
        // no-op
        break;
    }
  }

  editor->cancelBlame();

  QMainWindow::closeEvent(event);
}
