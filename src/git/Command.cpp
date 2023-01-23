//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Command.h"
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>

namespace git {

QString Command::bashPath() {
  QStringList paths;

#ifdef Q_OS_WIN
  QString git = QStandardPaths::findExecutable("git");
  if (git.isEmpty()) {
    QFileInfo info("C:/Program Files/git/cmd/git.exe");
    if (info.exists())
      git = info.path();
  }

  if (!git.isEmpty()) {
    QDir dir = QFileInfo(git).dir();
    dir.cdUp();
    paths.append(dir.filePath("bin"));
  }
#endif

  return QStandardPaths::findExecutable("bash", paths);
}

QString Command::substitute(const QProcessEnvironment &env,
                            const QString &command) {
  QList<QRegularExpressionMatch> matches;
  QRegularExpression re("\\$\\{?([_a-zA-Z]\\w+)\\}?");
  QRegularExpressionMatchIterator it = re.globalMatch(command);
  while (it.hasNext())
    matches.prepend(it.next());

  // Substitute in reverse order.
  QString result = command;
  foreach (const QRegularExpressionMatch &match, matches) {
    QString value = env.value(match.captured(1));
    result.replace(match.capturedStart(), match.capturedLength(), value);
  }

  return result;
}

} // namespace git
