//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Accounts.h"
#include "Beanstalk.h"
#include "Bitbucket.h"
#include "GitHub.h"
#include "Gitea.h"
#include "GitLab.h"
#include <QCoreApplication>
#include <QSettings>

Accounts::Accounts(QObject *parent) : QObject(parent) { load(); }

int Accounts::count() const { return mAccounts.size(); }

Account *Accounts::account(int index) const { return mAccounts.at(index); }

int Accounts::indexOf(Account *account) const {
  return mAccounts.indexOf(account);
}

Repository *Accounts::lookup(const QString &url) const {
  QUrl remote(url);
  foreach (Account *account, mAccounts) {
    for (int i = 0; i < account->repositoryCount(); ++i) {
      Repository *repo = account->repository(i);
      if (url == repo->url(Repository::Ssh))
        return repo;

      QUrl https(repo->url(Repository::Https));
      if (remote.host() == https.host() && remote.path() == https.path())
        return repo;
    }
  }

  return nullptr;
}

Account *Accounts::lookup(const QString &username, Account::Kind kind) const {
  foreach (Account *account, mAccounts) {
    if (username == account->username() && kind == account->kind())
      return account;
  }

  return nullptr;
}

Account *Accounts::createAccount(Account::Kind kind, const QString &username,
                                 const QString &url) {
  int index = mAccounts.size();

  Account *account = nullptr;
  switch (kind) {
    case Account::GitHub:
      account = new GitHub(username);
      break;
    case Account::Gitea:
      account = new Gitea(username);
      break;
    case Account::Bitbucket:
      account = new Bitbucket(username);
      break;
    case Account::Beanstalk:
      account = new Beanstalk(username);
      break;
    case Account::GitLab:
      account = new GitLab(username);
      break;
  }

  if (!account)
    return nullptr;

  account->setUrl(url);

  AccountProgress *progress = account->progress();
  connect(progress, &AccountProgress::valueChanged,
          [this, index](int value) { emit this->progress(index, value); });
  connect(progress, &AccountProgress::started,
          [this, index] { emit started(index); });
  connect(progress, &AccountProgress::finished, [this, index] {
    emit finished(index);
    store();
  });

  connect(account, &Account::repositoryAboutToBeAdded,
          [this, index] { emit repositoryAboutToBeAdded(index); });
  connect(account, &Account::repositoryAdded,
          [this, index] { emit repositoryAdded(index); });
  connect(account, &Account::repositoryPathChanged,
          [this, index](int repoIndex, const QString &path) {
            emit repositoryPathChanged(index, repoIndex, path);
          });

  emit accountAboutToBeAdded();
  mAccounts.append(account);
  emit accountAdded();

  return account;
}

void Accounts::removeAccount(int index) {
  emit accountAboutToBeRemoved();
  delete mAccounts.takeAt(index);
  emit accountRemoved();
  store();
}

void Accounts::removeAccount(Account *account) {
  int index = mAccounts.indexOf(account);
  if (index >= 0)
    removeAccount(index);
}

Accounts *Accounts::instance() {
  static Accounts *instance = nullptr;
  if (!instance)
    instance = new Accounts(qApp);
  return instance;
}

void Accounts::load() {
  qDeleteAll(mAccounts);
  mAccounts.clear();

  QSettings settings;
  int accountCount = settings.beginReadArray("accounts");
  for (int i = 0; i < accountCount; ++i) {
    settings.setArrayIndex(i);

    Account::Kind kind;

    {
      auto kindSetting = settings.value("kind");
      bool isNum;
      auto kindInt = kindSetting.toInt(&isNum);

      if (isNum) {
        kind = static_cast<Account::Kind>(kindInt);
      } else {
        bool ok;
        kind = Account::kindFromString(kindSetting.toString(), &ok);

        if (!ok) {
          continue;
        }
      }
    }

    QString username = settings.value("username").toString();
    QString url = settings.value("url").toString();
    Account *account = createAccount(kind, username, url);

    if (!account)
      continue;

    int repoCount = settings.beginReadArray("repos");
    for (int j = 0; j < repoCount; ++j) {
      settings.setArrayIndex(j);

      QString name = settings.value("name").toString();
      QString fullName = settings.value("full_name").toString();
      Repository *repo = account->addRepository(name, fullName);
      repo->setUrl(Repository::Https, settings.value("https_url").toString());
      repo->setUrl(Repository::Ssh, settings.value("ssh_url").toString());
    }
    settings.endArray();

    // Try to connect if no repos were loaded.
    if (!account->repositoryCount())
      account->connect();
  }
  settings.endArray();
}

void Accounts::store() {
  QSettings settings;
  settings.beginWriteArray("accounts");
  for (int i = 0; i < mAccounts.size(); ++i) {
    Account *account = mAccounts.at(i);
    settings.setArrayIndex(i);
    settings.setValue("kind", Account::kindToString(account->kind()));
    settings.setValue("username", account->username());
    if (account->hasCustomUrl()) {
      settings.setValue("url", account->url());
    } else {
      settings.remove("url");
    }

    settings.beginWriteArray("repos");
    for (int j = 0; j < account->repositoryCount(); ++j) {
      settings.setArrayIndex(j);

      Repository *repo = account->repository(j);
      settings.setValue("name", repo->name());
      settings.setValue("full_name", repo->fullName());
      settings.setValue("https_url", repo->url(Repository::Https));
      settings.setValue("ssh_url", repo->url(Repository::Ssh));
    }
    settings.endArray();
  }
  settings.endArray();
}
