//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Updater.h"
#include "cmark.h"
#include "DownloadDialog.h"
#include "UpdateDialog.h"
#include "UpToDateDialog.h"
#include "conf/Settings.h"
#include "ui/MainWindow.h"
#include "git/Command.h"
#include "Debug.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QProcess>
#include <QTemporaryFile>
#include <QUrl>
#include <QUrlQuery>
#include <QVersionNumber>

#if defined(Q_OS_MAC)
#define PLATFORM "macos"
#elif defined(Q_OS_WIN)
#if defined(Q_OS_WIN64)
#define PLATFORM "win64"
#else
#define PLATFORM "win32"
#endif
#else
#define PLATFORM "linux"
#endif

namespace {

const QString kDownloadPlatform = "Github";
const QString kTemplateFmt = "%1-XXXXXX.%2";
const QString kLinkFmt = "https://github.com/Murmele/gittyup/releases/latest/"
                         "download/Gittyup%1%2.%3";
const QString kChangelogUrl =
    "https://raw.githubusercontent.com/Murmele/Gittyup/gh-pages/changelog.md";

} // namespace

Updater::Download::Download(const QString &link) : mUrl(link) {
  QFileInfo info(name());
  QString name = kTemplateFmt.arg(info.completeBaseName(), info.suffix());
  mFile = new QTemporaryFile(QDir::temp().filePath(name));
}

Updater::Download::~Download() { delete mFile; }

Updater::Updater(QObject *parent) : QObject(parent) {
  // Set up connections.
  connect(&mMgr, &QNetworkAccessManager::sslErrors, this, &Updater::sslErrors);
  connect(this, &Updater::upToDate, [] {
    UpToDateDialog dialog;
    dialog.exec();
  });

  connect(this, &Updater::updateAvailable,
          [this](const QString &platform, const QString &version,
                 const QString &log, const QString &link) {
            // Show the update dialog.
            QVersionNumber appVersion = QVersionNumber::fromString(
                QCoreApplication::applicationVersion());
            QVersionNumber newVersion = QVersionNumber::fromString(version);

            if (Settings::instance()
                    ->value(Setting::Id::InstallUpdatesAutomatically)
                    .toBool()) {
              // Skip the update dialog and just start downloading.
              if (Updater::DownloadRef download = this->download(link)) {
                DownloadDialog *dialog = new DownloadDialog(download);
                dialog->show();
              }
            } else {
              UpdateDialog *dialog =
                  new UpdateDialog(platform, version, log, link);
              connect(dialog, &UpdateDialog::rejected, this,
                      &Updater::updateCanceled);
              dialog->show();
            }
          });

  connect(this, &Updater::updateError,
          [](const QString &text, const QString &detail) {
            QMessageBox mb(QMessageBox::Critical, tr("Update Failed"), text);
            mb.setInformativeText(detail);
            mb.exec();
          });
}

void Updater::update(bool spontaneous) {
  QNetworkRequest request(kChangelogUrl);
  QNetworkReply *reply = mMgr.get(request);
  connect(reply, &QNetworkReply::finished, [this, spontaneous, reply] {
    // Destroy the reply later.
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      if (!spontaneous)
        emit updateError(tr("Unable to check for updates"),
                         reply->errorString());
      return;
    }

    QByteArray changelog;
    QList<QByteArray> versions;
    QVersionNumber appVersion =
        QVersionNumber::fromString(QCoreApplication::applicationVersion());

    // Parse changelog.
    while (!reply->atEnd()) {
      QByteArray line = reply->readLine();
      QList<QByteArray> tokens = line.split(' ');
      if (tokens.size() > 1 && tokens.first() == "###") {
        QByteArray version = tokens.at(1).mid(1); // Strip 'v' prefix.
        if (QVersionNumber::fromString(version) <= appVersion)
          break;
        versions.append(version);
      }

      changelog.append(line);
    }

    if (versions.isEmpty()) {
      if (!spontaneous)
        emit upToDate();
      return;
    }

    // Check for skipped version.
    QString version = versions.first();
    QVariant skipped = Settings::instance()->value(Setting::Id::SkippedUpdates);
    if (spontaneous && skipped.toStringList().contains(version))
      return;

    // Strip trailing horizontal rule.
    while (changelog.endsWith('-') || changelog.endsWith('\n'))
      changelog.chop(1);

    char *ptr = cmark_markdown_to_html(changelog.data(), changelog.size(),
                                       CMARK_OPT_DEFAULT);
    QString html(ptr);
    free(ptr);

    // Build link.
    QString platform(PLATFORM);
    QString platformArg;
    QString extension = "sh";
#if defined(FLATPAK) || defined(DEBUG_FLATPAK)
    extension = "flatpak";
    platformArg = "";
    // The bundle does not have any version in its filename
    QString link = kLinkFmt.arg(platformArg, "", extension);
#else
	if (platform == "macos") {
	  extension = "dmg";
	} else if (platform.startsWith("win")) {
	  platformArg = QString("-%1").arg(platform);
	  extension = "exe";
	}
	QString link = kLinkFmt.arg(platformArg, QString("-%1").arg(version), extension);
#endif
    Debug("Download url of the update: " << link);

    // Check if the url exists
    auto *reply = mMgr.head(QNetworkRequest(link));
    connect(reply, &QNetworkReply::finished,
            [this, spontaneous, reply, platform, version, html, link] {
              // Destroy the reply later.
              reply->deleteLater();

              if (reply->error() == QNetworkReply::NoError) {
                emit updateAvailable(platform, version, html, link);
              } else {
                // We don't have a new version for this platform
                if (!spontaneous)
                  emit upToDate();
              }
            });
  });
}

Updater::DownloadRef Updater::download(const QString &link) {
  QString errorText = tr("Unable to download update");
  DownloadRef download = DownloadRef::create(link);
  if (!download->file()->open()) {
    emit updateError(errorText, tr("Unable to open temporary file"));
    return DownloadRef();
  }

  // Follow redirects.
  QNetworkRequest request(link);
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);

  QNetworkReply *reply = mMgr.get(request);
  connect(reply, &QNetworkReply::finished, [this, errorText, download] {
    // Destroy the reply later.
    QNetworkReply *reply = download->reply();
    reply->deleteLater();

    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError) {
      if (error == QNetworkReply::OperationCanceledError) {
        emit updateCanceled();
      } else {
        emit updateError(errorText, reply->errorString());
      }

      return;
    }

    // Write any remaining data.
    if (reply->isOpen()) {
      download->file()->write(reply->readAll());
      download->file()->close();
    }
  });

  // Write data to disk as soon as it becomes ready.
  connect(reply, &QNetworkReply::readyRead, [download] {
    download->file()->write(download->reply()->readAll());
  });

  download->setReply(reply);
  return download;
}

void Updater::install(const DownloadRef &download) {
  // First try to close all windows. Disable quit on close.
  bool quitOnClose = QGuiApplication::quitOnLastWindowClosed();
  QGuiApplication::setQuitOnLastWindowClosed(false);

  bool rejected = false;
  for (MainWindow *window : MainWindow::windows()) {
    rejected = !window->close();
    if (rejected) {
      break;
    }
  }

  QGuiApplication::setQuitOnLastWindowClosed(quitOnClose);

  QString errorText = tr("Unable to install update");
  if (rejected) {
    emit updateError(errorText,
                     tr("Some windows failed to close. You can "
                        "download the binary manually from %1")
                         .arg(QString("<a href=\"%1\">%2</a>")
                                  .arg(download->url(), kDownloadPlatform)));
    return;
  }

  // Try to install the new application.
  QString error = tr("Unknown install error");
  if (!install(download, error)) {
    emit updateError(errorText, error);
    return;
  }

  // Exit gracefully.
  QCoreApplication::quit();
}

Updater *Updater::instance() {
  static Updater *instance = nullptr;
  if (!instance)
    instance = new Updater(qApp);

  return instance;
}

#if defined(FLATPAK) || defined(DEBUG_FLATPAK)
bool Updater::uninstallGittyup(bool system) {
  QString bash = git::Command::bashPath();
  QString loc = system ? "--system" : "--user";

  QStringList args;
  args.append("-c");
  args.append(QString("flatpak-spawn --host flatpak remove -y %1 "
                      "com.github.Murmele.Gittyup")
                  .arg(loc));
  auto *p = new QProcess(this);

  p->start(bash, args);
  if (!p->waitForFinished()) {
    const QString es = p->errorString();
    qDebug() << "Uninstalling Gittyup failed: " + es;
    return false;
  } else {
    qDebug() << "Uninstall: " + p->readAll();
  }
  p->deleteLater();
  return true;
}

bool Updater::install(const DownloadRef &download, QString &error) {
  QString path = download->file()->fileName();

  // Ignore return value
  uninstallGittyup(true);
  uninstallGittyup(false);

  QDir dir(QCoreApplication::applicationDirPath());
  QStringList args;
  args.append("-c");
  args.append(
      QString("flatpak-spawn --host flatpak install --user -y %1").arg(path));
  Debug("Install arguments: " << args);
  Debug("Download file: " << path);
  QProcess *p = new QProcess(this);

  QString bash = git::Command::bashPath();
  Debug("Bash: " << bash);
  p->start(bash, args);
  if (!p->waitForFinished()) {
    const QString es = p->errorString();
    error = tr("Installer script failed: %1").arg(es);
    Debug("Installer script failed: " + es);
    return false;
  } else {
    Debug("Successfully installed bundle: " + p->readAll());
  }
  p->deleteLater();

  auto relauncher_cmd = dir.filePath("relauncher");
  Debug("Relauncher command: " << relauncher_cmd);

  // Start the relaunch helper.
  QString app = "flatpak-spawn --host flatpak run com.github.Murmele.Gittyup";
  QString pid = QString::number(QCoreApplication::applicationPid());
  if (!QProcess::startDetached(relauncher_cmd, {app, pid})) {
    error = tr("Helper application failed to start");
    return false;
  }

  return true;
}
#elif !defined(Q_OS_MAC) && !defined(Q_OS_WIN)
bool Updater::install(const DownloadRef &download, QString &error) {
  QString path = download->file()->fileName();
  QDir dir(QCoreApplication::applicationDirPath());
  QString prefix = QString("--prefix=%1").arg(dir.path());
  QStringList args({path, prefix, "--exclude-subdir"});
  if (int code = QProcess::execute("/bin/sh", args)) {
    error = tr("Installer script failed: %1").arg(code);
    return false;
  }

  // Start the relaunch helper.
  QString app = QCoreApplication::applicationFilePath();
  QString pid = QString::number(QCoreApplication::applicationPid());
  if (!QProcess::startDetached(dir.filePath("relauncher"), {app, pid})) {
    error = tr("Helper application failed to start");
    return false;
  }

  return true;
}
#endif
