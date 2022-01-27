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

EditTool::EditTool(const QString &file, QObject *parent)
  : ExternalTool(file, parent)
{}

bool EditTool::isValid() const
{
  return (ExternalTool::isValid() && QFileInfo(mFile).isFile());
}

ExternalTool::Kind EditTool::kind() const
{
  return Edit;
}

QString EditTool::name() const
{
  return tr("Edit in External Editor");
}

bool EditTool::start()
{
  git::Config config = git::Config::global();
  QString editor = config.value<QString>("gui.editor");

  if (editor.isEmpty())
    editor = qgetenv("GIT_EDITOR");

  if (editor.isEmpty())
    editor = config.value<QString>("core.editor");

  if (editor.isEmpty())
    editor = qgetenv("VISUAL");

  if (editor.isEmpty())
    editor = qgetenv("EDITOR");

  if (editor.isEmpty())
    return QDesktopServices::openUrl(QUrl::fromLocalFile(mFile));

  // Find arguments.
  QStringList args = editor.split("\" \"");

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
      // Format 4: Command (no argument)
    }
  }

  // Remove command, add filename, trim command.
  args.removeFirst();
  args.append(mFile);
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
