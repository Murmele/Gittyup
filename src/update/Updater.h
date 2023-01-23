//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef UPDATER_H
#define UPDATER_H

#include <QNetworkAccessManager>
#include <QSharedPointer>

class QNetworkReply;
class QTemporaryFile;

class Updater : public QObject {
  Q_OBJECT

public:
  class Download {
  public:
    Download(const QString &link);
    ~Download();

    QString name() const { return QUrl(url()).fileName(); }

    QTemporaryFile *file() { return mFile; }
    void setFile(QTemporaryFile *file) { mFile = file; }

    QNetworkReply *reply() const { return mReply; }
    void setReply(QNetworkReply *reply) { mReply = reply; }

    QString url() const { return mUrl; }

  private:
    QString mUrl;
    QTemporaryFile *mFile = nullptr;
    QNetworkReply *mReply = nullptr;
  };

  using DownloadRef = QSharedPointer<Download>;

  void update(bool spontaneous = false);
  DownloadRef download(const QString &link);
  void install(const DownloadRef &download);

  static Updater *instance();

signals:
  void upToDate();
  void updateCanceled();
  void updateError(const QString &text, const QString &detail);
  void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
  void updateAvailable(const QString &platform, const QString &version,
                       const QString &changelog, const QString &link);

private:
  Updater(QObject *parent = nullptr);

  bool install(const DownloadRef &download, QString &error);

#if defined(FLATPAK) || defined(DEBUG_FLATPAK)
  bool uninstallGittyup(bool system);
#endif

  QNetworkAccessManager mMgr;
};

#endif
