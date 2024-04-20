//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Repository.h"
#include "AnnotatedCommit.h"
#include "Blame.h"
#include "Branch.h"
#include "Command.h"
#include "Commit.h"
#include "Config.h"
#include "Filter.h"
#include "FilterList.h"
#include "Index.h"
#include "Patch.h"
#include "qtsupport.h"
#include "Rebase.h"
#include "Reference.h"
#include "Remote.h"
#include "RevWalk.h"
#include "Signature.h"
#include "Submodule.h"
#include "TagRef.h"
#include "Tree.h"
#include "util/Path.h"
#include "git2/buffer.h"
#include "git2/branch.h"
#include "git2/checkout.h"
#include "git2/cherrypick.h"
#include "git2/filter.h"
#include "git2/global.h"
#include "git2/ignore.h"
#include "git2/merge.h"
#include "git2/rebase.h"
#include "git2/refs.h"
#include "git2/remote.h"
#include "git2/repository.h"
#include "git2/signature.h"
#include "git2/stash.h"
#include "git2/tag.h"
#include "git2/sys/repository.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextCodec>
#include <QVector>

#ifdef Q_OS_UNIX
#include <pwd.h>
#include <unistd.h>
#endif

namespace git {

namespace {

const QString kConfigDir = "gittyup";
const QString kConfigFile = "config";
const QString kStarFile = "starred";

int blame_progress(const git_oid *suspect, void *payload) {
  return reinterpret_cast<Blame::Callbacks *>(payload)->progress() ? 0 : -1;
}

int checkout_notify(git_checkout_notify_t why, const char *path,
                    const git_diff_file *baseline, const git_diff_file *target,
                    const git_diff_file *workdir, void *payload) {
  char status = 'M';
  if (why == GIT_CHECKOUT_NOTIFY_CONFLICT) {
    status = '!';
  } else if (!baseline) {
    status = 'A';
  } else if (!target) {
    status = 'D';
  } else if (strcmp(baseline->path, target->path)) {
    // FIXME: Checkout doesn't actually do rename detection.
    status = 'R';
  }

  auto *cbs = reinterpret_cast<Repository::CheckoutCallbacks *>(payload);
  return cbs->notify(status, path) ? 0 : -1;
}

void checkout_progress(const char *path, size_t current, size_t total,
                       void *payload) {
  auto *cbs = reinterpret_cast<Repository::CheckoutCallbacks *>(payload);
  cbs->progress(path, current, total);
}

int insert_stash_id(size_t index, const char *message, const git_oid *id,
                    void *payload) {
  reinterpret_cast<QList<Id> *>(payload)->insert(index, id);
  return 0;
}

} // namespace

QMap<git_repository *, QWeakPointer<Repository::Data>> Repository::registry;

Repository::Data::Data(git_repository *repo)
    : repo(repo), notifier(new RepositoryNotifier) {
  // Load starred commits.
  QDir dir(git_repository_path(repo));
  QFile file(appDir(dir).filePath(kStarFile));
  if (!file.open(QIODevice::ReadOnly))
    return;

  QByteArray ids = file.readAll();
  if (ids.isEmpty())
    return;

  foreach (const QByteArray &id, ids.split('\n'))
    starredCommits.insert(QByteArray::fromHex(id));
}

Repository::Data::~Data() {
  delete notifier;
  git_repository_free(repo);
}

void Repository::unregisterRepository(Data *data) {
  registry.remove(data->repo);
  delete data;
}

QSharedPointer<Repository::Data>
Repository::registerRepository(git_repository *repo) {
  if (!repo)
    return QSharedPointer<Data>();

  auto it = registry.find(repo);
  if (it != registry.end())
    return *it;

  QSharedPointer<Data> ref(new Data(repo), unregisterRepository);
  registry[repo] = ref.toWeakRef();
  return ref;
}

Repository::Repository() {}

Repository::Repository(git_repository *repo) : d(registerRepository(repo)) {}

Repository::operator git_repository *() const { return d->repo; }

QDir Repository::dir(bool includeGitFolder) const {
  QDir dir(git_repository_path(d->repo));
  if (!includeGitFolder && dir.dirName() == ".git") {
    if (!dir.cdUp()) {
      assert(false); // must be done explicit, because in release build the
                     // assert will not be executed, so assert(dir.cdUp()) does
                     // not work in release build
    }
  }
  return dir;
}

QDir Repository::workdir() const {
  return isBare() ? dir() : QDir(git_repository_workdir(d->repo));
}

QDir Repository::appDir() const { return appDir(dir()); }

Id Repository::workdirId(const QString &path) const {
  git_oid id;
  if (git_blob_create_from_workdir(&id, d->repo, path.toUtf8()))
    return Id();

  return id;
}

QString Repository::message() const {
  git_buf buf = GIT_BUF_INIT_CONST(nullptr, 0);
  git_repository_message(&buf, d->repo);
  return QString::fromUtf8(buf.ptr, buf.size);
}

// Config file used for git specific configs
// config file in <Repository>/.git/config
Config Repository::gitConfig() const {
  git_config *config = nullptr;
  git_repository_config(&config, d->repo);
  return Config(config);
}

// Config file used for app specific configs
// config file in <Repository>/.git/gittyup/config
Config Repository::appConfig() const {
  Config config = Config::appGlobal();
  QString path = appDir().filePath(kConfigFile);
  config.addFile(path, GIT_CONFIG_LEVEL_LOCAL, d->repo);
  return config;
}

bool Repository::isBare() const { return git_repository_is_bare(d->repo); }

Signature Repository::signature(const QString &name, const QString &email) {
  return Signature(name, email);
}

Signature Repository::signature(const QString &name, const QString &email,
                                const QDateTime &date) {
  return Signature(name, email, date);
}

Signature Repository::defaultSignature(bool *fake, const QString &overrideUser,
                                       const QString &overrideEmail) const {
  QString name, email;

  if (fake)
    *fake = false;

  git_signature *signature = nullptr;
  if (!git_signature_default(&signature, d->repo)) {
    Signature res(signature, true);

    if (res.isValid() &&
        (!overrideUser.isEmpty() || !overrideEmail.isEmpty())) {
      if (overrideUser.isEmpty())
        name = res.name();
      else
        name = overrideUser;

      if (overrideEmail.isEmpty())
        email = res.email();
      else
        email = overrideEmail;

      git_signature_new(&signature, name.toUtf8(), email.toUtf8(),
                        res.gitDate().time, res.gitDate().offset);

      res = Signature(signature, true);
    }

    return res;
  }

#ifdef Q_OS_UNIX
  // Get user name.
  passwd *pw = getpwuid(getuid());
  name = pw->pw_gecos;

  // Create fake email address.
  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  email = QString("%1@%2").arg(pw->pw_name, hostname);
#else
  name = getenv("USERNAME");
  QString hostname = getenv("COMPUTERNAME");
  email = QString("%1@%2.local").arg(name, hostname);
#endif

  if (!overrideUser.isEmpty())
    name = overrideUser;
  if (!overrideEmail.isEmpty())
    email = overrideEmail;

  if (!git_signature_now(&signature, name.toUtf8(), email.toUtf8())) {
    if (fake)
      *fake = true;

    return Signature(signature, true);
  }

  return Signature();
}

bool Repository::isIgnored(const QString &path) const {
  int ignored = 0;
  git_ignore_path_is_ignored(&ignored, d->repo, path.toUtf8());
  return ignored;
}

Index Repository::index() const {
  git_index *index = nullptr;
  git_repository_index(&index, d->repo);
  return Index(index);
}

void Repository::setIndex(const Index &index) {
  git_repository_set_index(d->repo, index);
}

/*!
 * \brief Repository::status
 * Generates the diff from the index to the working directory
 * \param index
 * \param callbacks
 * \param ignoreWhitespace
 * \return
 */
Diff Repository::status(const Index &index, Diff::Callbacks *callbacks,
                        bool ignoreWhitespace) const {
  Tree tree;
  if (Reference ref = head()) {
    if (Commit commit = ref.target())
      tree = commit.tree();
  }

  Diff diff = diffTreeToIndex(tree, index, ignoreWhitespace);
  Diff workdir = diffIndexToWorkdir(index, callbacks, ignoreWhitespace);
  if (!diff.isValid() || !workdir.isValid())
    return Diff();

  diff.merge(workdir);
  diff.setIndex(index);

  return diff.count() ? diff : Diff();
}

Diff Repository::diffTreeToIndex(const Tree &tree, const Index &index,
                                 bool ignoreWhitespace) const {
  git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
  opts.flags |= GIT_DIFF_INCLUDE_TYPECHANGE;

  if (!appConfig().value<bool>("untracked.hide", false))
    opts.flags |= GIT_DIFF_INCLUDE_UNTRACKED | GIT_DIFF_RECURSE_UNTRACKED_DIRS;

  if (ignoreWhitespace)
    opts.flags |= GIT_DIFF_IGNORE_WHITESPACE;

  git_diff *diff = nullptr;
  git_diff_tree_to_index(&diff, d->repo, tree, index, &opts);
  return Diff(diff);
}

Diff Repository::diffIndexToWorkdir(const Index &index,
                                    Diff::Callbacks *callbacks,
                                    bool ignoreWhitespace) const {
  git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
  opts.flags |= (GIT_DIFF_DISABLE_MMAP | GIT_DIFF_INCLUDE_TYPECHANGE);

  if (!appConfig().value<bool>("untracked.hide", false))
    opts.flags |= GIT_DIFF_INCLUDE_UNTRACKED | GIT_DIFF_RECURSE_UNTRACKED_DIRS;

  if (ignoreWhitespace)
    opts.flags |= GIT_DIFF_IGNORE_WHITESPACE;

  if (callbacks) {
    opts.progress_cb = &Diff::Callbacks::progress;
    opts.payload = callbacks;
  }

  git_diff *diff = nullptr;
  git_diff_index_to_workdir(&diff, d->repo, index, &opts);
  return Diff(diff);
}

Reference Repository::head() const {
  git_reference *ref = nullptr;
  git_repository_head(&ref, d->repo);
  return Reference(ref);
}

bool Repository::isHeadUnborn() const {
  return git_repository_head_unborn(d->repo);
}

bool Repository::isHeadDetached() const {
  return git_repository_head_detached(d->repo);
}

QString Repository::unbornHeadName() const {
  git_reference *head = nullptr;
  if (git_reference_lookup(&head, d->repo, "HEAD"))
    return QString();

  QString name = git_reference_symbolic_target(head);
  git_reference_free(head);

  return name.section('/', -1);
}

bool Repository::setHead(const Reference &ref) {
  int error = git_repository_set_head(d->repo, ref.qualifiedName().toUtf8());
  emit d->notifier->referenceUpdated(head());
  return !error;
}

bool Repository::setHeadDetached(const Commit &commit) {
  int error = git_repository_set_head_detached(d->repo, commit);
  emit d->notifier->referenceUpdated(head());
  return !error;
}

QList<Reference> Repository::refs() const {
  git_reference_iterator *it = nullptr;
  if (git_reference_iterator_new(&it, d->repo))
    return QList<Reference>();

  QList<Reference> refs;
  git_reference *ref = nullptr;
  while (!git_reference_next(&ref, it))
    refs.append(Reference(ref));

  git_reference_iterator_free(it);

  return refs;
}

Reference Repository::lookupRef(const QString &name) const {
  if (name.isEmpty())
    return Reference();

  git_reference *ref = nullptr;
  git_reference_lookup(&ref, d->repo, name.toUtf8());
  return Reference(ref);
}

QList<Branch> Repository::branches(git_branch_t flags) const {
  git_branch_iterator *it = nullptr;
  if (git_branch_iterator_new(&it, d->repo, flags))
    return QList<Branch>();

  QList<Branch> branches;
  git_reference *ref = nullptr;
  git_branch_t type = GIT_BRANCH_LOCAL;
  while (!git_branch_next(&ref, &type, it))
    branches.append(Branch(ref));

  git_branch_iterator_free(it);

  return branches;
}

Branch Repository::lookupBranch(const QString &name, git_branch_t flags) const {
  if (name.isEmpty())
    return Branch();

  git_reference *branch = nullptr;
  git_branch_lookup(&branch, d->repo, name.toUtf8(), flags);
  return Branch(branch);
}

Branch Repository::createBranch(const QString &name, const Commit &target,
                                bool force) {
  Commit commit = target;
  if (!commit.isValid()) {
    Branch branch = head();
    if (!branch.isValid())
      return Branch();

    commit = branch.target();
    if (!commit.isValid())
      return Branch();
  }

  emit d->notifier->referenceAboutToBeAdded(name);

  git_reference *ref = nullptr;
  git_branch_create(&ref, d->repo, name.toUtf8(), commit, force);

  // We have to notify even if creation failed and the branch is invalid.
  // Clients can check the argument to see if a branch was really added.
  Branch branch(ref);
  emit d->notifier->referenceAdded(branch);

  return branch;
}

QList<TagRef> Repository::tags() const {
  git_reference_iterator *it = nullptr;
  if (git_reference_iterator_new(&it, d->repo))
    return QList<TagRef>();

  QList<TagRef> refs;
  git_reference *ref = nullptr;
  while (!git_reference_next(&ref, it)) {
    TagRef tag(ref);
    if (tag.isValid())
      refs.append(tag);
  }

  git_reference_iterator_free(it);

  return refs;
}

TagRef Repository::lookupTag(const QString &name) const {
  return lookupRef(QString("refs/tags/%1")
                       .arg(name)); // TODO: check if possible to use
                                    // GIT_REFS_TAGS_DIR instead of refs/tags
}

QStringList Repository::existingTags() const {

  git_strarray array;
  git_tag_list(&array, operator git_repository *());

  QStringList list;

  for (size_t i = 0; i < array.count; i++) {
    list.append(array.strings[i]);
  }

  git_strarray_dispose(&array);
  return list;
}

TagRef Repository::createTag(const Commit &target, const QString &name,
                             const QString &message, bool force,
                             const QString &overrideUser,
                             const QString &overrideEmail) {
  Signature signature;
  if (!message.isEmpty()) {
    signature = defaultSignature(nullptr, overrideUser, overrideEmail);
    if (!signature.isValid())
      return TagRef();
  }

  emit d->notifier->referenceAboutToBeAdded(name);

  git_oid id;
  if (signature.isValid()) {
    git_tag_create(&id, d->repo, name.toUtf8(), target, signature,
                   message.toUtf8(), force);
  } else {
    git_tag_create_lightweight(&id, d->repo, name.toUtf8(), target, force);
  }

  // FIXME: Tagging functions should pass out the new reference?
  TagRef tag = lookupTag(name);
  emit d->notifier->referenceAdded(tag);

  return tag;
}

Blob Repository::lookupBlob(const Id &id) const {
  git_object *obj = nullptr;
  git_object_lookup(&obj, d->repo, id, GIT_OBJECT_BLOB);
  return Blob(reinterpret_cast<git_blob *>(obj));
}

RevWalk Repository::walker(int sort) const {
  git_revwalk *revwalk = nullptr;
  if (git_revwalk_new(&revwalk, d->repo))
    return RevWalk();

  RevWalk walker(revwalk);
  git_revwalk_sorting(revwalk, sort);
  foreach (const Reference &ref, refs())
    git_revwalk_push_ref(revwalk, ref.qualifiedName().toUtf8());

  return walker;
}

Commit Repository::lookupCommit(const QString &prefix) const {
  git_oid id;
  git_oid_fromstrp(&id, prefix.toUtf8());

  git_commit *commit = nullptr;
  git_commit_lookup_prefix(&commit, d->repo, &id, prefix.length());
  return Commit(commit);
}

Commit Repository::lookupCommit(const Id &id) const {
  git_commit *commit = nullptr;
  git_commit_lookup(&commit, d->repo, id);
  return Commit(commit);
}

bool Repository::amend(const git::Commit &commitToAmend,
                       const git::Signature &author,
                       const git::Signature &committer,
                       const QString &commitMessage) {

  // Write the index tree.
  Index idx = index();
  if (!idx.isValid())
    return false;

  // Add new staged files to the amended commit
  Tree tree = idx.writeTree();
  if (!tree.isValid())
    return false;

  return commitToAmend.amend(author, committer, commitMessage, tree);
}

Commit Repository::commit(const QString &message,
                          const AnnotatedCommit &mergeHead, bool *fakeSignature,
                          const QString &overrideUser,
                          const QString &overrideEmail) {
  // Get the default signature for the repo.
  Signature signature =
      defaultSignature(fakeSignature, overrideUser, overrideEmail);

  return commit(signature, signature, message, mergeHead);
}

Commit Repository::commit(const Signature &author, const Signature &committer,
                          const QString &message,
                          const AnnotatedCommit &mergeHead) {
  if (!author.isValid() || !committer.isValid())
    return Commit();

  // Write the index tree.
  Index idx = index();
  if (!idx.isValid())
    return Commit();

  Tree tree = idx.writeTree();
  if (!tree.isValid())
    return Commit();

  // Lookup the parent commit.
  QVector<const git_commit *> parents;
  if (Reference ref = head()) {
    if (Commit commit = ref.target())
      parents.append(commit);
  }

  // Add merge head.
  if (mergeHead.isValid())
    parents.append(mergeHead.commit());

  // Create the commit.
  git_oid id;
  if (git_commit_create(&id, d->repo, "HEAD", author, committer, 0,
                        message.toUtf8(), tree, parents.size(), parents.data()))
    return Commit();

  // Cleanup merge state.
  switch (state()) {
    case GIT_REPOSITORY_STATE_NONE:
    case GIT_REPOSITORY_STATE_MERGE:
    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_CHERRYPICK:
      cleanupState();
      break;

    default:
      // no-op
      break;
  }

  git_commit *commit = nullptr;
  git_commit_lookup(&commit, d->repo, &id);
  emit d->notifier->referenceUpdated(head());
  return Commit(commit);
}

QList<Commit> Repository::starredCommits() const {
  QList<Commit> commits;
  foreach (const Id &id, d->starredCommits) {
    if (Commit commit = lookupCommit(id))
      commits.append(commit);
  }

  return commits;
}

bool Repository::isCommitStarred(const Id &commit) const {
  return d->starredCommits.contains(commit);
}

void Repository::setCommitStarred(const Id &commit, bool starred) {
  if (starred) {
    d->starredCommits.insert(commit);
  } else {
    d->starredCommits.remove(commit);
  }

  // Write to disk.
  QSaveFile file(appDir().filePath(kStarFile));
  if (!file.open(QIODevice::WriteOnly))
    return;

  QByteArrayList ids;
  foreach (const Id &id, d->starredCommits)
    ids.append(id.toByteArray().toHex());

  file.write(ids.join('\n'));
  file.commit();
}

void Repository::invalidateSubmoduleCache() {
  git_repository_submodule_cache_clear(d->repo);
  d->submodules.clear();
  d->submodulesCached = false;
}

QList<Submodule> Repository::submodules() const {
  ensureSubmodulesCached();

  QList<Submodule> submodules;
  foreach (const QString &name, d->submodules) {
    if (Submodule submodule = lookupSubmodule(name))
      submodules.append(submodule);
  }

  return submodules;
}

Submodule Repository::lookupSubmodule(const QString &name) const {
  ensureSubmodulesCached();

  git_submodule *submodule = nullptr;
  git_submodule_lookup(&submodule, d->repo, name.toUtf8());
  return Submodule(submodule);
}

Remote Repository::addRemote(const QString &name, const QString &url) {
  // FIXME: Validate name?

  emit d->notifier->remoteAboutToBeAdded(name);

  git_remote *remote = nullptr;
  git_remote_create(&remote, d->repo, name.toUtf8(), url.toUtf8());

  // We have to notify even if creation failed and the remote is invalid.
  // Clients can check the argument to see if a branch was really added.
  Remote result(remote);
  emit d->notifier->remoteAdded(result);

  return result;
}

void Repository::deleteRemote(const QString &name) {
  git_remote *remote = nullptr;
  if (git_remote_lookup(&remote, d->repo, name.toUtf8()))
    return;

  emit d->notifier->remoteAboutToBeRemoved(Remote(remote));

  git_remote_delete(d->repo, name.toUtf8());

  // We have to notify even if removal failed and the remote still exists.
  // Clients can lookup the remote by name to see if it was really removed.
  emit d->notifier->remoteRemoved(name);
}

Remote Repository::defaultRemote() const {
  Branch branch = head();
  if (branch.isValid()) {
    Remote remote = branch.remote();
    if (remote.isValid())
      return remote;
  }

  return lookupRemote("origin");
}

QList<Remote> Repository::remotes() const {
  git_strarray names;
  if (git_remote_list(&names, d->repo))
    return QList<Remote>();

  QList<Remote> remotes;
  for (size_t i = 0; i < names.count; ++i) {
    if (Remote remote = lookupRemote(names.strings[i]))
      remotes.append(remote);
  }

  git_strarray_dispose(&names);

  return remotes;
}

Remote Repository::lookupRemote(const QString &name) const {
  git_remote *remote = nullptr;
  git_remote_lookup(&remote, d->repo, name.toUtf8());
  return Remote(remote);
}

Remote Repository::anonymousRemote(const QString &url) const {
  git_remote *remote = nullptr;
  git_remote_create_anonymous(&remote, d->repo, url.toUtf8());
  return Remote(remote);
}

Reference Repository::stashRef() const { return lookupRef("refs/stash"); }

QList<Commit> Repository::stashes() const {
  QList<Id> ids;
  git_stash_foreach(d->repo, insert_stash_id, &ids);

  QList<Commit> commits;
  foreach (const Id &id, ids) {
    git_commit *commit = nullptr;
    if (!git_commit_lookup(&commit, d->repo, id))
      commits.append(Commit(commit));
  }

  return commits;
}

Commit Repository::stash(const QString &message) {
  Signature signature = defaultSignature();
  if (!signature.isValid())
    return Commit();

  git_oid id;
  QByteArray buffer = message.toUtf8();
  const char *msg = !buffer.isEmpty() ? buffer.constData() : nullptr;
  if (git_stash_save(&id, d->repo, signature, msg, GIT_STASH_DEFAULT))
    return Commit();

  git_commit *commit = nullptr;
  git_commit_lookup(&commit, d->repo, &id);
  emit d->notifier->referenceUpdated(stashRef());
  return Commit(commit);
}

bool Repository::applyStash(int index) {
  git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
  return !git_stash_apply(d->repo, index, &opts);
}

bool Repository::dropStash(int index) {
  // The stash reference goes away when this is the last stash.
  // Signal that the previous saved reference changed instead.
  Reference ref = stashRef();

  int error = git_stash_drop(d->repo, index);
  emit d->notifier->referenceUpdated(ref);
  return !error;
}

bool Repository::popStash(int index) {
  // The stash reference goes away when this is the last stash.
  // Signal that the previous saved reference changed instead.
  Reference ref = stashRef();

  git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
  int error = git_stash_pop(d->repo, index, &opts);
  emit d->notifier->referenceUpdated(ref);
  return !error;
}

Blame Repository::blame(const QString &name, const Commit &from,
                        Blame::Callbacks *callbacks) const {
  git_blame *blame = nullptr;
  git_blame_options options = GIT_BLAME_OPTIONS_INIT;
  if (from.isValid()) // Set start commit.
    options.newest_commit = *git_commit_id(from);
  if (callbacks) {
    options.progress_cb = blame_progress;
    options.payload = callbacks;
  }
  git_blame_file(&blame, d->repo, name.toUtf8(), &options);
  return Blame(blame, d->repo);
}

FilterList Repository::filters(const QString &path, const Blob &blob) const {
  git_filter_list *filters = nullptr;
  git_filter_list_load(&filters, d->repo, blob, path.toUtf8(),
                       GIT_FILTER_TO_WORKTREE, GIT_FILTER_DEFAULT);
  return FilterList(filters);
}

Commit Repository::mergeBase(const Commit &lhs, const Commit &rhs) const {
  git_oid id;
  if (git_merge_base(&id, d->repo, lhs, rhs))
    return Commit();

  git_commit *commit = nullptr;
  git_commit_lookup(&commit, d->repo, &id);
  return Commit(commit);
}

bool Repository::merge(const AnnotatedCommit &mergeHead) {
  int current = state();
  const git_annotated_commit *head = mergeHead;
  git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
  git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
  checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;
  int error = git_merge(d->repo, &head, 1, &mergeOpts, &checkoutOpts);
  if (state() != current)
    emit d->notifier->stateChanged();

  return !error;
}

/*!
 * \brief Repository::rebaseOpen
 * Open current rebase and return a Rebase object.
 * If no current rebase is ongoing an invalid Rebase object is returned
 * \return
 */
Rebase Repository::rebaseOpen() {
  git_rebase *rebase = nullptr;
  git_rebase_options opts = GIT_REBASE_OPTIONS_INIT; // TODO: check quite option
  git_rebase_open(&rebase, d->repo, &opts);
  return Rebase(d->repo, rebase);
}

void Repository::rebaseAbort() {

  Rebase r = rebaseOpen();
  if (r.isValid())
    r.abort();
}

// TODO: check that all arguments passed to the signals are valid when the
// RepoView gets the notification! (Using sharedpointer?)
void Repository::rebase(const AnnotatedCommit &mergeHead,
                        const QString &overrideUser,
                        const QString &overrideEmail) {
  git_rebase *r = nullptr;
  git_rebase_options opts = GIT_REBASE_OPTIONS_INIT;
  git_rebase_init(&r, d->repo, nullptr, mergeHead, nullptr, &opts);
  auto rebase = git::Rebase(d->repo, r, overrideUser, overrideEmail);

  if (!rebase.isValid()) {
    emit d->notifier->rebaseInitError();
    return;
  }

  // start rebasing
  rebaseContinue(QStringLiteral(""));
}

void Repository::rebaseContinue(const QString &commitMessage) {

  Rebase r = rebaseOpen();
  if (!r.isValid()) {
    // rebase anymore available. maybe rebased externally
    return;
  }

  if (r.currentIndex() != GIT_REBASE_NO_OPERATION) {
    // Rebase::next() was already called at leas once
    // externally or by a previous call of rebaseContinue

    // Check if it can be committed
    git::Commit c = r.commit(commitMessage);
    if (!c.isValid()) {
      emit d->notifier->rebaseConflict(r);
      return;
    } else {
      emit d->notifier->rebaseCommitSuccess(r, c, r.commitToRebase(),
                                            r.currentIndex() + 1);
      // Go on with the next rebase operation below
    }
  }
  // Loop over rebase operations.
  while (r.hasNext()) {
    git::Commit before = r.next();
    if (!before.isValid()) {
      emit d->notifier->rebaseCommitInvalid(r);
      rebaseAbort();
      return;
    }
    int currCommit =
        r.currentIndex() +
        1; // for showing to user it makes more sense starting from 1
    emit d->notifier->rebaseAboutToRebase(r, before, currCommit);

    QString message = before.message(); // use original message
    git::Commit after = r.commit(message);
    if (!after.isValid()) {
      emit d->notifier->rebaseConflict(r);
      return; // before ongoing, the user has to fix the conflicts.
    }

    emit d->notifier->rebaseCommitSuccess(r, after, before, currCommit);
  }

  if (r.finish())
    emit d->notifier->rebaseFinished(r);
  // TODO: implement
  // else
  // emit error
}

bool Repository::rebaseOngoing() {
  Rebase r = rebaseOpen();
  return r.isValid();
}

bool Repository::cherryPick(const Commit &commit) {
  int current = state();
  int error = git_cherrypick(d->repo, commit, nullptr);
  if (state() != current)
    emit d->notifier->stateChanged();

  return !error;
}

bool Repository::checkout(const Commit &commit, CheckoutCallbacks *callbacks,
                          const QStringList &paths, int strategy) {
  git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
  opts.checkout_strategy = strategy;

  if (callbacks) {
    opts.notify_flags = callbacks->flags();
    opts.notify_cb = checkout_notify;
    opts.notify_payload = callbacks;

    opts.progress_cb = checkout_progress;
    opts.progress_payload = callbacks;
  }

  QVector<char *> rawPaths;
  QVector<QByteArray> storage;
  if (!paths.isEmpty()) {
    // Paths are assumed to be exact matches.
    opts.checkout_strategy |= GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH;

    foreach (const QString &path, paths) {
      storage.append(path.toUtf8());
      rawPaths.append(storage.last().data());
    }

    opts.paths.count = rawPaths.size();
    opts.paths.strings = rawPaths.data();
  }

  git_commit *tmp = commit;
  git_object *obj = reinterpret_cast<git_object *>(tmp);
  return !git_checkout_tree(d->repo, obj, &opts);
}

int Repository::state() const { return git_repository_state(d->repo); }

void Repository::cleanupState() {
  int current = state();
  git_repository_state_cleanup(d->repo);
  if (state() != current) {
    Patch::clearConflictResolutions(d->repo);
    emit d->notifier->stateChanged();
  }
}

QTextCodec *Repository::codec() const {
  QString encoding = gitConfig().value<QString>("gui.encoding");
  QTextCodec *codec = QTextCodec::codecForName(encoding.toUtf8());
  return codec ? codec : QTextCodec::codecForLocale();
}

QString Repository::decode(const QByteArray &text) const {
  return codec()->toUnicode(text);
}

bool Repository::lfsIsInitialized() { return dir().exists("hooks/pre-push"); }

bool Repository::lfsInitialize() {
  return (!lfsExecute({"install"}).isEmpty() && lfsIsInitialized());
}

bool Repository::lfsDeinitialize() {
  return !lfsExecute({"uninstall", "--local"}).isEmpty();
}

QByteArray Repository::lfsSmudge(const QByteArray &lfsPointerText,
                                 const QString &file) {
  return lfsExecute({"smudge", file}, lfsPointerText);
}

QStringList Repository::lfsEnvironment() {
  return QString(lfsExecute({"env"})).split('\n');
}

Repository::LfsTracking Repository::lfsTracked() {
  QString output = lfsExecute({"track"});
  QStringList lines = output.split('\n', Qt::SkipEmptyParts);
  if (!lines.isEmpty())
    lines.removeFirst();

  QStringList tracked, excluded;
  bool excluding = false;
  foreach (const QString &line, lines) {
    if (line[0] != ' ') {
      excluding = true;
    } else if (excluding) {
      excluded.append(line.trimmed().section(' ', 0, 0));
    } else {
      tracked.append(line.trimmed().section(' ', 0, 0));
    }
  }

  return LfsTracking{tracked, excluded};
}

bool Repository::lfsSetTracked(const QString &pattern, bool tracked) {
  QStringList args;
  args.append(tracked ? "track" : "untrack");
  args.append(pattern);
  if (tracked)
    args.append("--lockable");

  return !lfsExecute(args).isEmpty();
}

QSet<QString> Repository::lfsLocks() {
  if (!d->lfsLocksCached) {
    QByteArray output = lfsExecute({"locks", "--json"});
    if (!output.isEmpty()) {
      QJsonDocument doc = QJsonDocument::fromJson(output);
      QJsonArray array = doc.array();
      for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array.at(i).toObject();
        QString path = obj.value("path").toString();
        d->lfsLocks.insert(path);
      }
    }

    d->lfsLocksCached = true;
  }

  return d->lfsLocks;
}

bool Repository::lfsIsLocked(const QString &path) {
  return lfsLocks().contains(path);
}

bool Repository::lfsSetLocked(const QString &path, bool locked) {
  if (lfsExecute({locked ? "lock" : "unlock", path}).isEmpty())
    return false;

  if (locked) {
    d->lfsLocks.insert(path);
  } else {
    d->lfsLocks.remove(path);
  }

  emit d->notifier->lfsLocksChanged();
  return true;
}

bool Repository::clean(const QString &name) {
  QDir dir = workdir();
  return (dir.remove(name) || (dir.cd(name) && dir.removeRecursively()));
}

int Repository::lastErrorKind() {
  const git_error *err = git_error_last();
  return err ? err->klass : GIT_ERROR_NONE;
}

QString Repository::lastError(const QString &defaultError) {
  if (!defaultError.isEmpty())
    return defaultError;

  const git_error *err = git_error_last();
  return err ? err->message : tr("Unknown error");
}

QDir Repository::appDir(const QDir &dir) {
  QDir app = dir;
  app.mkpath(kConfigDir);
  app.cd(kConfigDir);
  return app;
}

Repository Repository::init(const QString &path, bool bare) {
  git_repository *repo = nullptr;
  git_repository_init(&repo, util::canonicalizePath(path).toUtf8(), bare);
  return Repository(repo);
}

Repository Repository::open(const QString &path, bool searchParents) {
  git_repository *repo = nullptr;
  int flags = searchParents ? 0 : GIT_REPOSITORY_OPEN_NO_SEARCH;
  git_repository_open_ext(&repo, util::canonicalizePath(path).toUtf8(), flags,
                          nullptr);
  return Repository(repo);
}

void Repository::init() {
  git_libgit2_init();

  // Set global options.
  git_libgit2_opts(GIT_OPT_ENABLE_STRICT_HASH_VERIFICATION, false);

  // Load global filters.
  Filter::init();

#ifdef Q_OS_LINUX
  // FIXME: There has to be a better way...
  QStringList paths = {"/etc/ssl/certs/ca-certificates.crt",
                       "/etc/pki/tls/certs/ca-bundle.crt"};

  foreach (const QString &path, paths) {
    if (QFile::exists(path)) {
      git_libgit2_opts(GIT_OPT_SET_SSL_CERT_LOCATIONS,
                       path.toUtf8().constData(), NULL);
      break;
    }
  }
#endif
}

void Repository::shutdown() { git_libgit2_shutdown(); }

RepositoryNotifier::RepositoryNotifier(QObject *parent) : QObject(parent) {}

void Repository::ensureSubmodulesCached() const {
  if (!d->submodulesCached) {
    d->submodulesCached = true;
    git_submodule_foreach(
        d->repo,
        [](git_submodule *, const char *name, void *payload) {
          reinterpret_cast<QStringList *>(payload)->append(name);
          return 0;
        },
        &d->submodules);
    git_repository_submodule_cache_all(d->repo);
  }
}

QByteArray Repository::lfsExecute(const QStringList &args,
                                  const QByteArray &input) const {
  QString path = QStandardPaths::findExecutable("git-lfs");
  if (path.isEmpty()) {
    emit d->notifier->lfsNotFound();
    git_error_set_str(GIT_ERROR_INVALID, tr("git-lfs not found").toUtf8());
    return QByteArray();
  }

  QProcess process;
  process.setWorkingDirectory(workdir().path());
  process.start(path, args);
  if (!input.isEmpty()) {
    process.write(input);
    process.closeWriteChannel();
  }

  process.waitForFinished();
  if (process.exitCode() != 0) {
    git_error_set_str(GIT_ERROR_INVALID, process.readAllStandardError());
    return QByteArray();
  }

  return process.readAllStandardOutput();
}

} // namespace git
