//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REMOTE_H
#define REMOTE_H

#include "Repository.h"
#include "Result.h"
#include "git2/net.h"
#include "git2/proxy.h"
#include <QSet>
#include <QString>
#include <QSharedPointer>

struct git_cred;
struct git_oid;
struct git_remote;
struct git_indexer_progress;

namespace git {

class Id;
class Reference;

class Remote {
public:
  struct PushUpdate {
    QByteArray srcName;
    QByteArray dstName;
    Id srcId;
    Id dstId;
  };

  struct SshInteractivePrompt {
    QString text;
    bool echo;
  };

  class Callbacks {
  public:
    enum State { Transfer, Resolve, Update };

    Callbacks(const QString &url, const Repository &repo = Repository())
        : mUrl(url), mRepo(repo) {}

    QString url() const { return mUrl; }

    State state() const { return mState; }

    Repository repo() const { return mRepo; }

    virtual void sideband(const QString &text) {}

    virtual bool credentials(const QString &url, QString &username,
                             QString &password) {
      return true;
    }

    virtual void interactiveAuth(const QString &name,
                                 const QString &instruction,
                                 const QVector<SshInteractivePrompt> &prompts,
                                 QVector<QString> &responses) {}

    virtual bool transfer(int total, int current, size_t bytes) { return true; }

    virtual bool resolve(int total, int current) { return true; }

    virtual void update(const QString &name, const Id &a, const Id &b) {}

    virtual void rejected(const QString &name, const QString &status) {}

    virtual void add(int total, int current) {}

    virtual void delta(int total, int current) {}

    // pre-push hook
    virtual bool negotiation(const QList<PushUpdate> &updates) { return true; }

    // url resolution hook
    virtual bool url(QString &url) { return true; }

    // user setting hooks
    virtual QString keyFilePath() const { return QString(); }
    virtual QString configFilePath() const { return QString(); }

    // agent availability hook
    virtual bool connectToAgent() const { return false; }

    // static callback wrappers
	static void connected(git_remote *remote, void *payload);

	static void about_to_disconnect(git_remote *remote, void *payload);

    static int sideband(const char *str, int len, void *payload);

    static int credentials(git_credential **out, const char *url,
                           const char *name, unsigned int types, void *payload);

    static int certificate(git_cert *cert, int valid, const char *host,
                           void *payload);

    static int transfer(const git_indexer_progress *stats, void *payload);

    static int update(const char *name, const git_oid *a, const git_oid *b,
                      void *payload);

	static int remoteReady(git_remote *remote,
						   int direction,
						   void *payload);

  protected:
    // Try to stop the current remote.
    void stop();

    QString mUrl;
    Repository mRepo;
    State mState = Transfer;
    QSet<QString> mAgentNames;
    git_remote *mRemote = nullptr;
    QSet<QString> mKeyFiles;
  };

  Remote();

  bool isValid() const { return !d.isNull(); }
  explicit operator bool() const { return isValid(); }

  QString name() const;
  void setName(const QString &name);

  QString url() const;
  void setUrl(const QString &url);

  Result fetch(Callbacks *callbacks, bool tags = false, bool prune = false);
  Result push(Callbacks *callbacks, const QStringList &refspecs);
  Result push(Callbacks *callbacks, const Reference &src,
              const QString &dst = QString(), bool force = false,
              bool tags = false);

  static Result clone(Callbacks *callbacks, const QString &url,
                      const QString &path, bool bare = false);

  static QByteArray proxyUrl(const QString &url, git_proxy_t &type);

  static bool isLoggingEnabled();
  static void setLoggingEnabled(bool enabled);
  static void log(const QString &text);

private:
  Remote(git_remote *remote);

  QSharedPointer<git_remote> d;

  friend class Branch;
  friend class Repository;
};

} // namespace git

Q_DECLARE_METATYPE(git::Remote);

#endif
