//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ShowTool.h"
#include "conf/Settings.h"
#include "git/Repository.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

#if defined(Q_OS_MAC)
#define NAME QT_TRANSLATE_NOOP("ShowTool", "Finder")
#elif defined(Q_OS_WIN)
#define NAME QT_TRANSLATE_NOOP("ShowTool", "Explorer")
#else
#define NAME QT_TRANSLATE_NOOP("ShowTool", "Default File Browser")
#endif

bool ShowTool::openFileManager(QString path) {
  QString fileManagerCmd =
      Settings::instance()->value(Setting::Id::FilemanagerCommand).toString();

  if (fileManagerCmd.isEmpty()) {
#if defined(Q_OS_WIN)
    fileManagerCmd = "explorer \"%1\"";

#elif defined(Q_OS_MACOS)
    fileManagerCmd = "open \"%1\"";

#elif defined(Q_OS_UNIX)
    fileManagerCmd = "xdg-open \"%1\"";
#endif
  }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  QStringList cmdParts = QProcess::splitCommand(fileManagerCmd);
#else
  /* Source:
   * https://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/io/qprocess.cpp */
  QStringList cmdParts;
  QString tmp;
  int quoteCount = 0;
  bool inQuote = false;

  // handle quoting. tokens can be surrounded by double quotes
  // "hello world". three consecutive double quotes represent
  // the quote character itself.
  for (int i = 0; i < fileManagerCmd.size(); ++i) {
    if (fileManagerCmd.at(i) == QLatin1Char('"')) {
      ++quoteCount;
      if (quoteCount == 3) {
        // third consecutive quote
        quoteCount = 0;
        tmp += fileManagerCmd.at(i);
      }
      continue;
    }
    if (quoteCount) {
      if (quoteCount == 1)
        inQuote = !inQuote;
      quoteCount = 0;
    }
    if (!inQuote && fileManagerCmd.at(i).isSpace()) {
      if (!tmp.isEmpty()) {
        cmdParts += tmp;
        tmp.clear();
      }
    } else {
      tmp += fileManagerCmd.at(i);
    }
  }
  if (!tmp.isEmpty())
    cmdParts += tmp;
#endif
  path = QDir::toNativeSeparators(path);

  for (QString &part : cmdParts)
    part = part.arg(path);

#if defined(FLATPAK)
  QStringList arguments;
  arguments << "--host" << cmdParts;
  return QProcess::startDetached("flatpak-spawn", arguments);
#else
  QString program = cmdParts.takeFirst();
  return QProcess::startDetached(program, cmdParts);
#endif
}

ShowTool::ShowTool(const QStringList &files, const git::Diff &diff,
                   const git::Repository &repo, QObject *parent)
    : ExternalTool(files, diff, repo, parent) {}

ExternalTool::Kind ShowTool::kind() const { return Show; }

QString ShowTool::name() const { return tr("Show in %1").arg(tr(NAME)); }

bool ShowTool::start() {

  foreach (const QString &file, mFiles) {
#if defined(Q_OS_MAC)
    if (!QProcess::startDetached("/usr/bin/osascript",
                                 {"-e", "tell application \"Finder\"", "-e",
                                  QString("reveal POSIX file \"%1\"").arg(file),
                                  "-e", "activate", "-e", "end tell"})) {
      return false;
    }
#elif defined(Q_OS_WIN)
    if (!QProcess::startDetached(
            "explorer.exe", {"/select,", QDir::toNativeSeparators(file)})) {
      return false;
    }
#else
    QFileInfo info(file);
    if (!openFileManager(info.isDir() ? info.filePath() : info.path())) {
      return false;
    }
#endif
  }

  return true;
}
