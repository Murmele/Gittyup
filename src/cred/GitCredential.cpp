//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "GitCredential.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QTextStream>
#include <QUrl>

namespace {

QString host(const QString &url) {
  QString host = QUrl(url).host();
  if (!host.isEmpty())
    return host;

  // Extract hostname from SSH URL.
  int end = url.indexOf(':');
  int begin = url.indexOf('@') + 1;
  return url.mid(begin, end - begin);
}

QString protocol(const QString &url) {
  QString scheme = QUrl(url).scheme();
  return !scheme.isEmpty() ? scheme : "ssh";
}

} // namespace

GitCredential::GitCredential(const QString &name) : mName(name) {}

bool GitCredential::get(const QString &url, QString &username,
                        QString &password) {
  QProcess process;
  process.start(command(), {"get"});
  if (!process.waitForStarted())
    return false;

  QTextStream out(&process);
  out << "protocol=" << protocol(url) << endl;
  out << "host=" << host(url) << endl;
  if (!username.isEmpty())
    out << "username=" << username << endl;
  out << endl;

  process.closeWriteChannel();
  process.waitForFinished();

  QString output = process.readAllStandardOutput();
  foreach (const QString &line, output.split('\n')) {
    int pos = line.indexOf('=');
    if (pos < 0)
      continue;

    QString key = line.left(pos);
    QString value = line.mid(pos + 1);
    if (key == "username") {
      username = value;
    } else if (key == "password") {
      password = value;
    }
  }

  return !username.isEmpty() && !password.isEmpty();
}

bool GitCredential::store(const QString &url, const QString &username,
                          const QString &password) {
  QProcess process;
  process.start(command(), {"store"});
  if (!process.waitForStarted())
    return false;

  QTextStream out(&process);
  out << "protocol=" << protocol(url) << endl;
  out << "host=" << host(url) << endl;
  out << "username=" << username << endl;
  out << "password=" << password << endl;
  out << endl;

  process.closeWriteChannel();
  process.waitForFinished();

  return true;
}

QString GitCredential::command() const {
  QString name = QString("git-credential-%1").arg(mName);
  // Prefer credential helpers directly installed into Gittyup's app dir
  QString candidate = QStandardPaths::findExecutable(
      name, QStringList(QCoreApplication::applicationDirPath()));
  if (!candidate.isEmpty()) {
    return candidate;
  }

  candidate = QStandardPaths::findExecutable(name);
  if (!candidate.isEmpty()) {
    return candidate;
  }

#ifdef Q_OS_WIN
  // Look for GIT CLI installation path
  QString gitPath = QStandardPaths::findExecutable("git");
  if (!gitPath.isEmpty()) {
    QDir gitDir = QFileInfo(gitPath).dir();
    if (gitDir.dirName() == "cmd" || gitDir.dirName() == "bin") {
      gitDir.cdUp();

#ifdef Q_OS_WIN64
      gitDir.cd("mingw64");
#else
      gitDir.cd("mingw32");
#endif

      gitDir.cd("bin");

      candidate =
          QStandardPaths::findExecutable(name, QStringList(gitDir.path()));
      if (!candidate.isEmpty()) {
        return candidate;
      }
    }
  }
#endif

  return name;
}
