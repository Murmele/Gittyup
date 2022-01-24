//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REMOTECALLBACKS_H
#define REMOTECALLBACKS_H

#include "git/Remote.h"
#include "git/Repository.h"
#include "host/Account.h"
#include <QElapsedTimer>
#include <QObject>
#include <QSet>

class LogEntry;

class RemoteCallbacks : public QObject, public git::Remote::Callbacks
{
  Q_OBJECT

public:
  enum Kind
  {
    Send,
    Receive
  };

  RemoteCallbacks(
    Kind kind,
    LogEntry *log,
    const QString &url,
    const QString &name = QString(),
    QObject *parent = nullptr,
    const git::Repository &repo = git::Repository());

  bool isCanceled() const { return mCanceled; }
  void setCanceled(bool canceled);

  void storeDeferredCredentials();

  bool credentials(
    const QString &url,
    QString &username,
    QString &password) override;

  virtual void interactiveAuth(
    const QString &name,
    const QString &instruction,
    const QVector<git::Remote::SshInteractivePrompt> &prompts,
    QVector<QString> &responses
  ) override;

  void sideband(const QString &text) override;
  bool transfer(int total, int current, int bytes) override;
  bool resolve(int total, int current) override;
  void update(const QString &name, const git::Id &a, const git::Id &b) override;
  void rejected(const QString &name, const QString &status) override;
  void add(int total, int current) override;
  void delta(int total, int current) override;

  // pre-push hook
  bool negotiation(const QList<git::Remote::PushUpdate> &updates) override;

  // user setting hooks
  QString keyFilePath() const override;
  QString configFilePath() const override;

  // agent availability hook
  bool connectToAgent() const override;

signals:
  void referenceUpdated(const QString &name);

  // These are implementation details.
  void queueCredentials(
    const QString &url,
    QString &username,
    QString &password,
    QString &error);

  void queueInteractiveAuth(
    const QString &name,
    const QString &instruction,
    const QVector<git::Remote::SshInteractivePrompt> &prompts,
    QVector<QString> &responses,
    QString &error
  );
  void queueSideband(const QString &text, const QString &fmt = QString());
  void queueTransfer(int total, int current, int bytes, int elapsed);
  void queueResolve(int total, int current);
  void queueUpdate(const QString &name, const git::Id &a, const git::Id &b);
  void queueRejected(const QString &name, const QString &status);
  void queueAdd(int total, int current);
  void queueDelta(int total, int current);

private:
  void credentialsImpl(
    const QString &url,
    QString &username,
    QString &password,
    QString &error);
  void interactiveAuthImpl(
    const QString &name,
    const QString &instruction,
    const QVector<git::Remote::SshInteractivePrompt> &prompts,
    QVector<QString> &responses,
    QString &error
  );
  void sidebandImpl(const QString &text, const QString &fmt = QString());
  void transferImpl(int total, int current, int bytes, int elapsed);
  void resolveImpl(int total, int current);
  void updateImpl(const QString &name, const git::Id &a, const git::Id &b);
  void rejectedImpl(const QString &name, const QString &status);
  void addImpl(int total, int current);
  void deltaImpl(int total, int current);

  Kind mKind;
  LogEntry *mLog;
  QString mName;

  QElapsedTimer mTimer;
  QString mSideband;
  int mBytesReceived = 0;
  bool mCanceled = false;

  LogEntry *mSidebandItem = nullptr;
  LogEntry *mTransferItem = nullptr;
  LogEntry *mResolveItem = nullptr;
  LogEntry *mAddItem = nullptr;
  LogEntry *mDeltaItem = nullptr;

  QSet<QStringList> mQueriedCredentials;

  QString mDeferredUrl;
  QString mDeferredUsername;
  QString mDeferredPassword;
};

#endif
