//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include "Account.h"
#include <QObject>

class Accounts : public QObject {
  Q_OBJECT

public:
  int count() const;
  Account *account(int index) const;
  int indexOf(Account *account) const;
  Repository *lookup(const QString &url) const;
  Account *lookup(const QString &username, Account::Kind kind) const;

  Account *createAccount(Account::Kind kind, const QString &username,
                         const QString &url = QString());
  void removeAccount(int index);
  void removeAccount(Account *account);

  static Accounts *instance();

signals:
  void accountAboutToBeAdded();
  void accountAdded();
  void accountAboutToBeRemoved();
  void accountRemoved();

  void progress(int index, int value);
  void started(int index);
  void finished(int index);

  void repositoryAboutToBeAdded(int index);
  void repositoryAdded(int index);
  void repositoryPathChanged(int index, int repoIndex, const QString &path);

private:
  Accounts(QObject *parent = nullptr);

  void load();
  void store();

  QList<Account *> mAccounts;
};

#endif
