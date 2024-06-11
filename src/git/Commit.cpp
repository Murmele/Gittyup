//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Commit.h"
#include "Diff.h"
#include "Patch.h"
#include "Reference.h"
#include "Repository.h"
#include "RevWalk.h"
#include "Signature.h"
#include "TagRef.h"
#include "Tree.h"
#include "git2/annotated_commit.h"
#include "git2/diff.h"
#include "git2/refs.h"
#include "git2/revert.h"
#include "git2/commit.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextCodec>
#include <QVector>

namespace git {

namespace {

QString sEmojiFile;
QMap<QString, QString> sEmojiCache;

} // namespace

Commit::Commit() : Object() {}

Commit::Commit(git_commit *commit)
    : Object(reinterpret_cast<git_object *>(commit)) {}

Commit::operator git_commit *() const {
  return reinterpret_cast<git_commit *>(d.data());
}

bool Commit::isMerge() const { return git_commit_parentcount(*this) > 1; }

QString Commit::summary(MessageOptions options) const {
  QString msg = decodeMessage(git_commit_summary(*this));
  return (options & SubstituteEmoji) ? substituteEmoji(msg) : msg;
}

QString Commit::body(MessageOptions options) const {
  QString msg = decodeMessage(git_commit_body(*this));
  return (options & SubstituteEmoji) ? substituteEmoji(msg) : msg;
}

QString Commit::message(MessageOptions options) const {
  QString msg = decodeMessage(git_commit_message(*this));
  return (options & SubstituteEmoji) ? substituteEmoji(msg) : msg;
}

QString Commit::description() const {
  // Build list of candidates.
  QHash<Id, TagRef> candidates;
  foreach (const TagRef &tag, repo().tags()) {
    if (Commit commit = tag.target())
      candidates.insert(commit.id(), tag);
  }

  if (candidates.isEmpty())
    return QString();

  // Check for exact match.
  auto it = candidates.constFind(id());
  if (it != candidates.constEnd())
    return it->name();

  // Walk parents.
  QList<Commit> commits = parents();
  while (!commits.isEmpty()) {
    QSet<Commit> parents;
    foreach (const Commit &commit, commits) {
      auto it = candidates.constFind(commit.id());
      if (it != candidates.constEnd())
        return QString("%1 +%2").arg(it->name()).arg(difference(commit));

      foreach (const Commit &parent, commit.parents())
        parents.insert(parent);
    }

    commits = parents.values();
  }

  return QString();
}

QString Commit::detachedHeadName() const {
  foreach (const TagRef &tag, repo().tags()) {
    if (tag.target().id() == id())
      return tag.name();
  }

  return shortId();
}

AnnotatedCommit Commit::annotatedCommit() const {
  git_annotated_commit *commit = nullptr;
  git_repository *repo = git_object_owner(d.data());
  git_annotated_commit_lookup(&commit, repo, git_object_id(d.data()));
  return AnnotatedCommit(commit, repo);
}

Signature Commit::author() const {
  return const_cast<git_signature *>(git_commit_author(*this));
}

Signature Commit::committer() const {
  return const_cast<git_signature *>(git_commit_committer(*this));
}

Diff Commit::diff(const git::Commit &commit, int contextLines,
                  bool ignoreWhitespace) const {
  Tree old;
  if (commit.isValid()) {
    old = commit.tree();
  } else {
    QList<Commit> parents = this->parents();
    if (!parents.isEmpty())
      old = parents.first().tree();
  }

  git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
  opts.context_lines = contextLines;
  opts.flags |= GIT_DIFF_INCLUDE_TYPECHANGE;
  if (ignoreWhitespace)
    opts.flags |= GIT_DIFF_IGNORE_WHITESPACE;

  git_diff *diff = nullptr;
  git_repository *repo = git_object_owner(d.data());
  git_diff_tree_to_tree(&diff, repo, old, tree(), &opts);
  return Diff(diff);
}

Tree Commit::tree() const {
  git_tree *tree = nullptr;
  git_commit_tree(&tree, *this);
  return Tree(tree);
}

QList<Commit> Commit::parents() const {
  QList<Commit> list;
  int count = git_commit_parentcount(*this);
  for (int i = 0; i < count; ++i) {
    git_commit *parent = 0;
    if (git_commit_parent(&parent, *this, i))
      continue; // FIXME: Report error?

    list.append(Commit(parent));
  }

  return list;
}

QList<Reference> Commit::refs() const {
  // Add detached HEAD.
  QList<Reference> refs;
  Repository repo = this->repo();
  if (repo.isHeadDetached()) {
    Reference head = repo.head();
    if (head.isValid() && head.target().id() == id())
      refs.append(head);
  }

  // Iterate over references.
  git_reference_iterator *it = nullptr;
  if (git_reference_iterator_new(&it, repo))
    return QList<Reference>();

  git_reference *ref = nullptr;
  const git_oid *id = git_object_id(d.data());
  while (!git_reference_next(&ref, it)) {
    git_object *obj = nullptr;
    if (!git_reference_peel(&obj, ref, GIT_OBJECT_COMMIT) &&
        git_oid_equal(git_object_id(obj), id)) {
      refs.append(Reference(ref));
    } else {
      git_reference_free(ref);
    }

    git_object_free(obj);
  }

  git_reference_iterator_free(it);

  return refs;
}

RevWalk Commit::walker(int sort, bool firstCommitOnly) const {
  git_revwalk *revwalk = nullptr;
  if (git_revwalk_new(&revwalk, git_object_owner(d.data())))
    return RevWalk();

  RevWalk walker(revwalk);
  if (git_revwalk_push(revwalk, git_object_id(d.data())))
    return RevWalk();

  if (firstCommitOnly && git_revwalk_simplify_first_parent(revwalk))
    return RevWalk();

  git_revwalk_sorting(revwalk, sort);

  return walker;
}

int Commit::difference(const Commit &commit) const {
  // Check for the same commit.
  if (id() == commit.id())
    return 0;

  RevWalk walk = walker();
  if (!walk.isValid() || !walk.hide(commit))
    return 0;

  int result = 0;
  while (walk.next().isValid())
    ++result;

  return result;
}

bool Commit::revert() const {
  Repository repo = this->repo();
  int state = repo.state();
  git_revert_options opts = GIT_REVERT_OPTIONS_INIT;
  int error = git_revert(git_object_owner(d.data()), *this, &opts);
  if (repo.state() != state)
    emit repo.notifier()->stateChanged();

  return !error;
}

bool Commit::amend(const Signature &author, const Signature &committer,
                   const QString &commitMessage, const Tree &tree) const {
  Repository repo = this->repo();
  git_oid oid;
  int error = git_commit_amend(&oid, *this, "HEAD", &*author, &*committer, NULL,
                               commitMessage.toUtf8(), tree);
  emit repo.notifier()->referenceUpdated(repo.head());
  return !error;
}

bool Commit::reset(git_reset_t type, const QStringList &paths,
                   bool triggerReferenceUpdated) const {
  QVector<char *> rawPaths;
  QVector<QByteArray> storage;
  git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
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

  Repository repo = this->repo();
  int state = repo.state();
  int error = git_reset(repo, d.data(), type, &opts);
  if (triggerReferenceUpdated)
    emit repo.notifier()->referenceUpdated(repo.head(),
                                           type == git_reset_t::GIT_RESET_HARD);
  if (repo.state() != state) {
    Patch::clearConflictResolutions(repo);
    emit repo.notifier()->stateChanged();
  }

  return !error;
}

bool Commit::isStarred() const { return repo().isCommitStarred(id()); }

void Commit::setStarred(bool starred) {
  repo().setCommitStarred(id(), starred);
}

QString Commit::decodeMessage(const char *msg) const {
  if (const char *encoding = git_commit_message_encoding(*this)) {
    if (QTextCodec *codec = QTextCodec::codecForName(encoding))
      return codec->toUnicode(msg);
  }

  return msg;
}

QString Commit::substituteEmoji(const QString &text) const {
  if (sEmojiFile.isEmpty())
    return text;

  // Populate cache.
  if (sEmojiCache.isEmpty()) {
    QFile file(sEmojiFile);
    if (file.open(QFile::ReadOnly)) {
      QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
      for (const QJsonValue &val : doc.array()) {
        QJsonObject obj = val.toObject();
        QString emoji = obj.value("emoji").toString();
        if (!emoji.isEmpty()) {
          for (const QJsonValue &alias : obj.value("aliases").toArray())
            sEmojiCache[alias.toString()] = emoji;
        }
      }
    }
  }

  // Build list of matches.
  QList<QRegularExpressionMatch> matches;
  QRegularExpression re(":((\\w|_)+):");
  QRegularExpressionMatchIterator it = re.globalMatch(text);
  while (it.hasNext())
    matches.prepend(it.next());

  // Substitute in reverse order.
  QString result = text;
  foreach (const QRegularExpressionMatch &match, matches) {
    auto it = sEmojiCache.constFind(match.captured(1));
    if (it != sEmojiCache.constEnd())
      result.replace(match.capturedStart(), match.capturedLength(), it.value());
  }

  return result;
}

void Commit::setEmojiFile(const QString &file) { sEmojiFile = file; }

Blob Commit::blob(const QString &file) const {
  git::Blob blob;

  if (!isValid())
    return blob;

  // searching for the correct blob
  auto list = file.split("/");
  bool found = false;
  auto t = tree();
  if (!t.isValid())
    return blob;

  for (int path_depth = 0; path_depth < list.count(); path_depth++) {
    auto element = list[path_depth];
    found = false;
    for (int i = 0; i < t.count(); ++i) {
      auto n = t.name(i);
      if (n == element) {
        if (path_depth >= list.count() - 1)
          blob = t.object(i);
        else
          t = t.object(i);
        found = true;
        break;
      }
    }
    if (!found)
      break;
  }
  return blob;
}

} // namespace git
