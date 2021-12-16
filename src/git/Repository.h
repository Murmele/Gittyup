//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "AnnotatedCommit.h"
#include "Blame.h"
#include "Blob.h"
#include "Commit.h"
#include "Diff.h"
#include "Index.h"
#include "git2/checkout.h"
#include "git2/errors.h"
#include "git2/revwalk.h"
#include "git2/types.h"
#include <QCoreApplication>
#include <QDir>
#include <QObject>
#include <QSet>
#include <QSharedPointer>

struct git_repository;
class QProcess;

namespace git {

class Branch;
class Config;
class FilterList;
class Id;
class Rebase;
class Reference;
class Remote;
class RepositoryNotifier;
class RevWalk;
class Signature;
class Submodule;
class TagRef;

class Repository
{
  Q_DECLARE_TR_FUNCTIONS(Repository)

public:
  class CheckoutCallbacks
  {
  public:
    virtual int flags() const
    {
      return GIT_CHECKOUT_NOTIFY_NONE;
    }

    virtual bool notify(char status, const QString &path)
    {
      return false;
    }

    virtual void progress(const QString &path, int current, int total) {}
  };

  Repository();

  bool isValid() const { return !d.isNull(); }
  explicit operator bool() const { return isValid(); }

  RepositoryNotifier *notifier() const { return d->notifier; }

  QDir dir() const;
  QDir workdir() const;
  QDir appDir() const;

  Id workdirId(const QString &path) const;

  // default message
  QString message() const;

  // config
  Config config() const;
  Config appConfig() const;

  // bare
  bool isBare() const;

  // signature
  Signature defaultSignature(bool *fake = nullptr) const;

  // ignore
  bool isIgnored(const QString &path) const;

  // index
  Index index() const;
  void setIndex(const Index &index);

  // status/diff
  Diff status(
    const Index &index,
    Diff::Callbacks *callbacks,
    bool ignoreWhitespace = false) const;
  Diff diffTreeToIndex(
    const Tree &tree,
    const Index &index = Index(),
    bool ignoreWhitespace = false) const;
  Diff diffIndexToWorkdir(
    const Index &index = Index(),
    Diff::Callbacks *callbacks = nullptr,
    bool ignoreWhitespace = false) const;

  // refs
  QList<Reference> refs() const;
  Reference lookupRef(const QString &name) const;

  Reference head() const;
  bool isHeadUnborn() const;
  bool isHeadDetached() const;
  QString unbornHeadName() const;
  bool setHead(const Reference &ref);
  bool setHeadDetached(const Commit &commit);

  // branch
  QList<Branch> branches(git_branch_t flags = GIT_BRANCH_ALL) const;
  Branch lookupBranch(
    const QString &name,
    git_branch_t flags = GIT_BRANCH_ALL) const;
  Branch createBranch(
    const QString &name,
    const Commit &target = Commit(),
    bool force = false);

  // tag
  QList<TagRef> tags() const;
  TagRef lookupTag(const QString &name) const;
  TagRef createTag(
    const Commit &target,
    const QString &name,
    const QString &message = QString(),
    bool force = false);
  QStringList existingTags() const;

  // blob
  Blob lookupBlob(const Id &id) const;

  // commit
  RevWalk walker(int sort = GIT_SORT_NONE) const;
  Commit lookupCommit(const QString &prefix) const;
  Commit lookupCommit(const Id &id) const;
  Commit commit(
    const QString &message,
    const AnnotatedCommit &mergeHead = AnnotatedCommit(),
    bool *fakeSignature = nullptr);

  QList<Commit> starredCommits() const;
  bool isCommitStarred(const Id &commit) const;
  void setCommitStarred(const Id &commit, bool starred);

  // submodule
  void invalidateSubmoduleCache();
  QList<Submodule> submodules() const;
  Submodule lookupSubmodule(const QString &path) const;

  // remote
  Remote addRemote(const QString &name, const QString &url);
  void deleteRemote(const QString &name);
  Remote defaultRemote() const;
  QList<Remote> remotes() const;
  Remote lookupRemote(const QString &name) const;
  Remote anonymousRemote(const QString &url) const;

  // stash
  Reference stashRef() const;
  QList<Commit> stashes() const;
  Commit stash(const QString &message = QString());
  bool applyStash(int index = 0);
  bool dropStash(int index = 0);
  bool popStash(int index = 0);

  // blame
  Blame blame(
    const QString &name,
    const Commit &from,
    Blame::Callbacks *callbacks = nullptr) const;

  // filter
  FilterList filters(const QString &path, const Blob &blob = Blob()) const;

  // merge/rebase
  Commit mergeBase(const Commit &lhs, const Commit &rhs) const;
  bool merge(const AnnotatedCommit &mergeHead);
  Rebase rebase(const AnnotatedCommit &mergeHead);

  // cherry-pick
  bool cherryPick(const Commit &commit);

  // checkout
  bool checkout(const Commit &commit,
    CheckoutCallbacks *callbacks = nullptr,
    const QStringList &paths = QStringList(),
    int strategy = GIT_CHECKOUT_SAFE, const QString &targetDirectory = "");

  // Clean up after merge/rebase/cherry-pick/etc.
  int state() const;
  void cleanupState();

  // encoding
  QTextCodec *codec() const;
  QString decode(const QByteArray &text) const;

  // clean
  bool clean(const QString &name);

  // LFS
  bool lfsIsInitialized();
  bool lfsInitialize();
  bool lfsDeinitialize();

  QByteArray lfsSmudge(const QByteArray &lfsPointerText, const QString &file);

  QStringList lfsEnvironment();
  QStringList lfsTracked();
  bool lfsSetTracked(const QString &pattern, bool tracked);

  QSet<QString> lfsLocks();
  bool lfsIsLocked(const QString &path);
  bool lfsSetLocked(const QString &path, bool locked);

  // last error
  static int lastErrorKind();
  static QString lastError(const QString &defaultError = QString());

  // Get the app dir for the given git dir.
  static QDir appDir(const QDir &dir);

  // Open the git repository at path.
  static Repository init(const QString &path, bool bare = false);
  static Repository open(const QString &path, bool searchParents = false);

  static void init();
  static void shutdown();

private:
  struct Data
  {
    Data(git_repository *repo);
    ~Data();

    git_repository *repo;
    RepositoryNotifier *notifier;

    QStringList submodules;
    bool submodulesCached = false;

    QSet<QString> lfsLocks;
    bool lfsLocksCached = false;

    QSet<Id> starredCommits;
  };

  Repository(git_repository *repo);
  operator git_repository *() const;

  void ensureSubmodulesCached() const;

  QByteArray lfsExecute(
    const QStringList &args,
    const QByteArray &input = QByteArray()) const;

  static void unregisterRepository(Data *data);
  static QSharedPointer<Data> registerRepository(git_repository *repo);

  QSharedPointer<Data> d;

  static QMap<git_repository *,QWeakPointer<Data>> registry;

  friend class Branch;
  friend class Commit;
  friend class Config;
  friend class Index;
  friend class Object;
  friend class Patch;
  friend class Rebase;
  friend class Reference;
  friend class Remote;
  friend class Submodule;
  friend class TagRef;
};

class RepositoryNotifier : public QObject
{
  Q_OBJECT

public:
  RepositoryNotifier(QObject *parent = nullptr);

signals:
  void referenceAboutToBeAdded(const QString &name);
  void referenceAdded(const Reference &ref);
  void referenceAboutToBeRemoved(const Reference &ref);
  void referenceRemoved(const QString &name);
  void referenceUpdated(const Reference &ref);

  void remoteAboutToBeAdded(const QString &name);
  void remoteAdded(const Remote &remote);
  void remoteAboutToBeRemoved(const Remote &remote);
  void remoteRemoved(const QString &name);

  void stateChanged();
  void workdirChanged();

  void directoryStaged();
  void directoryAboutToBeStaged(
    const QString &dir, int count, bool &allow);
  void largeFileAboutToBeStaged(
    const QString &path, int size, bool &allow);
  void indexChanged(const QStringList &paths, bool yieldFocus = true);
  void indexStageError(const QString &path);

  void lfsNotFound();
  void lfsLocksChanged();
};

} // namespace git

#endif
