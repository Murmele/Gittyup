//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "EditTool.h"
#include "git/Config.h"
#include "git/Repository.h"
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>

EditTool::EditTool(const QStringList &files, const git::Diff &diff,
           const git::Repository &repo, QObject *parent)
    : ExternalTool(files, diff, repo, parent) {}

bool EditTool::isValid() const {
  if (!ExternalTool::isValid()) return false;

  foreach (const QString file, mFiles) {
    if (!QFileInfo(mRepo.workdir().filePath(file)).isFile()) return false;
  }
  return true;
}

ExternalTool::Kind EditTool::kind() const { return Edit; }

QString EditTool::name() const { return tr("Edit in External Editor"); }

bool EditTool::start() {
  git::Config config = git::Config::global();
  QString baseEditor = config.value<QString>("gui.editor");

  if (baseEditor.isEmpty())
    baseEditor = qgetenv("GIT_EDITOR");

  if (baseEditor.isEmpty())
    baseEditor = config.value<QString>("core.editor");

  if (baseEditor.isEmpty())
    baseEditor = qgetenv("VISUAL");

  if (baseEditor.isEmpty())
    baseEditor = qgetenv("EDITOR");

  if (baseEditor.isEmpty()) {
    foreach (const QString &file, mFiles) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(file));
    }
    return true;
  }

  QString editor = baseEditor;

  // Find arguments.
  QStringList args = baseEditor.split("\" \"");
  if (args.count() > 1) {
    // Format 1: "Command" "Argument1" "Argument2"
    editor = args[0];
    for (int i = 1; i < args.count(); i++)
      args[i].remove("\"");
  } else {
    int fi = editor.indexOf("\"");
    int li = editor.lastIndexOf("\"");
    if ((fi == 0) && (li > fi) && (li < (editor.length() - 1))) {
      // Format 2: "Command" Argument1 Argument2
      args = editor.right(editor.length() - li - 2).split(" ");
      args.insert(0, "dummy");
      editor = editor.left(li + 1);
    } else {
      // Format 3: "Command" (no argument)
      if (fi == -1) {
        // Format 4: Command  Argument1 Argument2
        // Format 5: Command (no argument)
        args = editor.split(" ");
        editor = args.size() ? args[0] : "";
      }
    }
  }

  // Remove command, add filename, trim command.
  args.removeFirst();
  foreach (const QString &file, mFiles) {
    args.append(mRepo.workdir().filePath(file));
  }
  editor.remove("\"");

  // Destroy this after process finishes.
  QProcess *process = new QProcess(this);
  auto signal = QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished);
  QObject::connect(process, signal, this, &ExternalTool::deleteLater);

#if defined(FLATPAK)
  args.prepend(editor);
  args.prepend("--host");
  process->start("flatpak-spawn", args);
#else
  process->start(editor, args);
#endif

  if (!process->waitForStarted())
    return false;

  // Detach from parent.
  setParent(nullptr);

  return true;
}
