//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "GitLab.h"
#include "Repository.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

namespace {

const QString kContentType = "application/json";
const QString kProjectsFmt = "/projects?membership=true&private_token=%1";

} // namespace

GitLab::GitLab(const QString &username) : Account(username) {
  QObject::connect(
      mMgr, &QNetworkAccessManager::finished, this,
      [this](QNetworkReply *reply) {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
          setErrorReply(*reply);
          mProgress->finish();
          return;
        }

        // Read repositories.
        QJsonArray array = QJsonDocument::fromJson(reply->readAll()).array();
        for (int i = 0; i < array.size(); ++i) {
          QJsonObject obj = array.at(i).toObject();
          QString name = obj.value("path").toString();
          QString fullName = obj.value("path_with_namespace").toString();
          QString httpUrl = obj.value("http_url_to_repo").toString();
          QString sshUrl = obj.value("ssh_url_to_repo").toString();

          Repository *repo = addRepository(name, fullName);
          repo->setUrl(Repository::Https, httpUrl);
          repo->setUrl(Repository::Ssh, sshUrl);
        }

        // Check for additional pages.
        QString link = reply->rawHeader("Link");
        if (link.isEmpty()) {
          mProgress->finish();
          return;
        }

        QMap<QString, QString> map;
        QRegularExpression re("<(.*)>; rel=\"(\\w+)\"");
        foreach (const QString &record, link.split(", ")) {
          QRegularExpressionMatch match = re.match(record);
          if (match.isValid() && match.hasMatch())
            map.insert(match.captured(2), match.captured(1));
        }

        QString next = map.value("next");
        if (next.isEmpty()) {
          mProgress->finish();
          return;
        }

        // Request next page.
        QNetworkRequest request(next);
        request.setHeader(QNetworkRequest::ContentTypeHeader, kContentType);
        mMgr->get(request);
        startProgress();
      });
}

Account::Kind GitLab::kind() const { return Account::GitLab; }

QString GitLab::name() const { return QStringLiteral("GitLab"); }

QString GitLab::host() const { return QStringLiteral("gitlab.com"); }

void GitLab::connect(const QString &defaultPassword) {
  QString token = defaultPassword;
  if (token.isEmpty())
    token = password();

  if (token.isEmpty()) {
    mError->setText(tr("Authentication failed"));
    return;
  }

  clearRepos();

  QNetworkRequest request(url() + kProjectsFmt.arg(token));
  request.setHeader(QNetworkRequest::ContentTypeHeader, kContentType);
  mMgr->get(request);
  startProgress();
}

QString GitLab::defaultUrl() {
  return QStringLiteral("https://gitlab.com/api/v4");
}
