//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Bitbucket.h"
#include "Repository.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

const QString kSshFmt = "git@bitbucket.org:%1";
const QString kContentType = "application/json";
const char *kPasswordProperty = "password";

} // namespace

Bitbucket::Bitbucket(const QString &username) : Account(username) {
  QObject::connect(
      mMgr, &QNetworkAccessManager::finished, this,
      [this](QNetworkReply *reply) {
        QString password = reply->property(kPasswordProperty).toString();
        if (password.isEmpty())
          return;

        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
          setErrorReply(*reply);
          mProgress->finish();
          return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject jsonObject = doc.object();
        QJsonArray array = jsonObject["values"].toArray();
        for (int i = 0; i < array.size(); ++i) {
          QJsonObject obj = array.at(i).toObject();
          QJsonObject repository = obj.value("repository").toObject();

          QString name = repository.value("name").toString();
          QString fullName = repository.value("full_name").toString();

          QUrl httpsUrl;
          httpsUrl.setHost(host());
          httpsUrl.setScheme("https");
          httpsUrl.setUserName(this->username());
          httpsUrl.setPath(QString("/%1").arg(fullName));

          Repository *repo = addRepository(name, fullName);
          repo->setUrl(Repository::Https, httpsUrl.toString());
          repo->setUrl(Repository::Ssh, kSshFmt.arg(fullName));
        }

        QString next = jsonObject["next"].toString();
        if (next.isEmpty()) {
          mProgress->finish();
          return;
        }

        // Request next page.
        QNetworkRequest request(next);
        if (setHeaders(request, password)) {
          QNetworkReply *reply = mMgr->get(request);
          reply->setProperty(kPasswordProperty, password);
          startProgress();
        }
      });
}

Account::Kind Bitbucket::kind() const { return Account::Bitbucket; }

QString Bitbucket::name() const { return QStringLiteral("Bitbucket"); }

QString Bitbucket::host() const { return QStringLiteral("bitbucket.org"); }

void Bitbucket::connect(const QString &password) {
  clearRepos();

  QNetworkRequest request(
      url() +
      "/user/permissions/repositories?sort=repository.name&pagelen=100");
  request.setHeader(QNetworkRequest::ContentTypeHeader, kContentType);

  if (setHeaders(request, password)) {
    QNetworkReply *reply = mMgr->get(request);
    reply->setProperty(kPasswordProperty,
                       !password.isEmpty() ? password : this->password());
    startProgress();
  }
}

QString Bitbucket::defaultUrl() {
  return QStringLiteral("https://api.bitbucket.org/2.0");
}
