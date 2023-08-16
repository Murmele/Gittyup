//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Account.h"
#include "Beanstalk.h"
#include "Bitbucket.h"
#include "GitHub.h"
#include "Gitea.h"
#include "GitLab.h"
#include "cred/CredentialHelper.h"
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QNetworkRequest>
#include <QPalette>
#include <QSettings>
#include <qnetworkaccessmanager.h>

namespace {

const QString kIconFmt = ":/%1.png";
const QString kThemeIconFmt = ":/%1_%2.png";

} // namespace

Account::Account(const QString &username)
    : mMgr(new QNetworkAccessManager()), mUsername(username),
      mError(new AccountError(this)), mProgress(new AccountProgress(this)) {
  QObject::connect(
      mMgr, &QNetworkAccessManager::sslErrors,
      [this](QNetworkReply *reply, const QList<QSslError> &errors) {
        sslErrors = errors;
      });
}

Account::~Account() { mMgr->deleteLater(); }

QString Account::password() const {
  QUrl url;
  url.setScheme("https");
  url.setHost(host());

  QString password;
  QString name = username();
  CredentialHelper::instance()->get(url.toString(), name, password);

  return password;
}

bool Account::hasCustomUrl() const { return !mUrl.isEmpty(); }

QString Account::url() const {
  QString url = hasCustomUrl() ? mUrl : defaultUrl(kind());
  return url.replace("<username>", username());
}

void Account::setUrl(const QString &url) { mUrl = url; }

int Account::repositoryCount() const { return mRepos.size(); }

int Account::indexOf(Repository *repo) const { return mRepos.indexOf(repo); }

Repository *Account::repository(int index) const { return mRepos.at(index); }

Repository *Account::addRepository(const QString &name,
                                   const QString &fullName) {
  emit repositoryAboutToBeAdded();
  Repository *repo = new Repository(name, fullName, this);
  mRepos.append(repo);
  emit repositoryAdded();
  return repo;
}

QString Account::repositoryPath(int index) const {
  if (index >= mRepos.size())
    return QString();

  if (!mRepoPaths.contains(index)) {
    QSettings settings;
    settings.beginGroup("remote");
    settings.beginGroup(name().toLower());
    QString key = mRepos.at(index)->fullName();
    QString path = settings.value(key).toString();
    if (!path.isEmpty() && !QFileInfo(path).exists()) {
      settings.remove(key);
      path = QString();
    }
    mRepoPaths.insert(index, path);
    settings.endGroup();
    settings.endGroup();
  }

  return mRepoPaths.value(index);
}

void Account::setRepositoryPath(int index, const QString &path) {
  mRepoPaths.insert(index, path);

  QSettings settings;
  settings.beginGroup("remote");
  settings.beginGroup(name().toLower());
  QString key = mRepos.at(index)->fullName();
  if (path.isEmpty()) {
    settings.remove(key);
  } else {
    settings.setValue(key, path);
  }
  settings.endGroup();
  settings.endGroup();

  emit repositoryPathChanged(index, path);
}

void Account::setErrorReply(const QNetworkReply &reply) {
  QString details = reply.errorString();

  for (auto error : sslErrors) {
    details += "\n" + error.errorString();
  }

  mError->setText(tr("Connection failed"), details);
}

void Account::authorize() { Q_ASSERT(false); }

bool Account::isAuthorizeSupported() { return false; }

QIcon Account::icon(Kind kind) {
  QString name;
  switch (kind) {
    case Account::GitHub:
      name = "github";
      break;
    case Account::Gitea:
      name = "gitea";
      break;
    case Account::Bitbucket:
      name = "bitbucket";
      break;
    case Account::Beanstalk:
      name = "beanstalk";
      break;
    case Account::GitLab:
      name = "gitlab";
      break;
  }

  QPalette palette = QGuiApplication::palette();
  QColor base = palette.color(QPalette::Base);
  QColor text = palette.color(QPalette::Text);
  if (text.lightnessF() > base.lightnessF()) {
    QFileInfo info(kThemeIconFmt.arg(name, "dark"));
    if (info.exists())
      return QIcon(info.filePath());
  }

  return QIcon(kIconFmt.arg(name));
}

QString Account::name(Kind kind) {
  switch (kind) {
    case Account::GitHub:
      return "GitHub";
    case Account::Gitea:
      return "Gitea";
    case Account::Bitbucket:
      return "Bitbucket";
    case Account::Beanstalk:
      return "Beanstalk";
    case Account::GitLab:
      return "GitLab";
  }

  return QString();
}

QString Account::helpText(Kind kind) {
  switch (kind) {
    case GitHub:
      return tr(
          "<b>Note:</b> Basic authentication is not supported if you have "
          "two-factor authentication enabled. Use a <a href='https://help."
          "github.com/articles/creating-a-personal-access-token-for-the-"
          "command-line/'>personal access token</a> in the password field "
          "instead.");

    case Gitea:
      return tr(
          "<b>Note:</b> Only Basic authentication is currently supported ");

    case GitLab:
      return tr("<b>Note:</b> Basic authentication is not supported. Use a "
                "<a href='https://docs.gitlab.com/ee/user/profile/personal_"
                "access_tokens.html'>personal access token</a> in the password "
                "field instead.");

    case Bitbucket:
    case Beanstalk:
      return QString();

    default:
      throw std::runtime_error("Unhandled case or invalid enum " + std::to_string(static_cast<int>(kind)));
  }
}

QString Account::defaultUrl(Kind kind) {
  switch (kind) {
    case GitHub:
      return GitHub::defaultUrl();
    case Gitea:
      return Gitea::defaultUrl();
    case Bitbucket:
      return Bitbucket::defaultUrl();
    case Beanstalk:
      return Beanstalk::defaultUrl();
    case GitLab:
      return GitLab::defaultUrl();
    default:
      throw std::runtime_error("Unhandled case or invalid enum " + std::to_string(static_cast<int>(kind)));
  }
}

Account::Kind Account::kindFromString(const QString &kind, bool *ok) {
  if (ok != nullptr) {
    *ok = true;
  }

  if (kind == "github") {
    return GitHub;
  } else if (kind == "gitea") {
    return Gitea;
  } else if (kind == "bitbucket") {
    return Bitbucket;
  } else if (kind == "beanstalk") {
    return Beanstalk;
  } else if (kind == "gitlab") {
    return GitLab;
  } else {
    if (ok != nullptr) {
      *ok = false;
    }

    return (Kind)0;
  }
}

QString Account::kindToString(Kind kind) {
  switch (kind) {
    case GitHub:
      return "github";
    case Gitea:
      return "gitea";
    case Bitbucket:
      return "bitbucket";
    case Beanstalk:
      return "beanstalk";
    case GitLab:
      return "gitlab";
    default:
      throw std::runtime_error("Unhandled case or invalid enum " + std::to_string(static_cast<int>(kind)));
  }
}

void Account::startProgress() {
  mError->setText(QString());
  mProgress->start();
}

void Account::clearRepos() {
  qDeleteAll(mRepos);
  mRepos.clear();
  mRepoPaths.clear();
}

bool Account::setHeaders(QNetworkRequest &request,
                         const QString &defaultPassword) {
  // If no password was provided then try to look one up from the keychain.
  QString password = defaultPassword;
  if (password.isEmpty())
    password = this->password();

  if (password.isEmpty()) {
    mError->setText(tr("Authentication failed"));
    return false;
  }

  QString cred = QString("%1:%2").arg(username(), password);
  request.setRawHeader("Authorization", "Basic " + cred.toUtf8().toBase64());

  // For the time being at least, all supported hosts understand JSON.
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  // Identify the application.
  QString name = QCoreApplication::applicationName();
  QString version = QCoreApplication::applicationVersion();
  QString agent = QString("%1/%2").arg(name, version);
  request.setHeader(QNetworkRequest::UserAgentHeader, agent);
  return true;
}

AccountError::AccountError(Account *parent) : QObject(parent) {}

Account *AccountError::account() const {
  return static_cast<Account *>(parent());
}

void AccountError::setText(const QString &text, const QString &detailedText) {
  mText = text;
  mDetailedText = detailedText;
}

AccountProgress::AccountProgress(Account *parent) : QObject(parent) {
  // Connect progress timer.
  QObject::connect(&mTimer, &QTimer::timeout,
                   [this] { emit valueChanged(++mValue); });
}

Account *AccountProgress::account() const {
  return static_cast<Account *>(parent());
}

void AccountProgress::start() {
  mTimer.start(50);
  mValue = 0;
  emit started();
}

void AccountProgress::finish() {
  mTimer.stop();
  mValue = -1;
  emit finished();
}
