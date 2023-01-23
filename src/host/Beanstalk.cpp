//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Beanstalk.h"
#include "Repository.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

Beanstalk::Beanstalk(const QString &username) : Account(username) {
  QObject::connect(
      mMgr, &QNetworkAccessManager::finished, this,
      [this](QNetworkReply *reply) {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
          QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
          QJsonArray array = doc.array();
          for (int i = 0; i < array.size(); ++i) {
            QJsonObject obj = array.at(i).toObject();
            obj = obj.value("repository").toObject();
            if (obj.value("vcs").toString() != "git")
              continue;

            QString name = obj.value("name").toString();
            QString httpsUrl = obj.value("repository_url_https").toString();
            QString sshUrl = obj.value("repository_url").toString();

            Repository *repo = addRepository(name, name);
            repo->setUrl(Repository::Https, httpsUrl);
            repo->setUrl(Repository::Ssh, sshUrl);
          }
        } else {
          setErrorReply(*reply);
        }

        mProgress->finish();
      });
}

Account::Kind Beanstalk::kind() const { return Account::Beanstalk; }

QString Beanstalk::name() const { return QStringLiteral("Beanstalk"); }

QString Beanstalk::host() const {
  return QStringLiteral("%1.git.beanstalkapp.com").arg(username());
}

void Beanstalk::connect(const QString &password) {
  clearRepos();

  QNetworkRequest request(url() + "/api/repositories.json");
  if (setHeaders(request, password)) {
    mMgr->get(request);
    startProgress();
  }
}

QString Beanstalk::defaultUrl() {
  return QStringLiteral("https://<username>.beanstalkapp.com");
}
