//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RevWalk.h"
#include "Commit.h"
#include "Reference.h"
#include "git2/commit.h"
#include "git2/pathspec.h"
#include "git2/revwalk.h"
#include "git2/version.h"
#include <QRegularExpression>

namespace git {

// Copies of a RevWalk share the walk, so its filter state lives here too.
struct RevWalk::Data {
  explicit Data(git_revwalk *walk) : walk(walk) {}
  ~Data() {
    git_revwalk_free(walk);
#if LIBGIT2_VERSION_CHECK(2, 0, 0)
    git_pathspec_free(pathspec);
#endif
  }

  git_revwalk *walk;
#if LIBGIT2_VERSION_CHECK(2, 0, 0)
  // Borrowed by the walk, so it must outlive it.
  git_pathspec *pathspec = nullptr;
#else
  QString path;
#endif
};

#if !LIBGIT2_VERSION_CHECK(2, 0, 0)
namespace {

int notify(const git_diff *, const git_diff_delta *, const char *, void *) {
  // Abort the diff.
  return -1;
}

// True if the commit's tree differs from the given parent's on the pathspec.
bool differs(git_commit *commit, unsigned int index,
             const git_diff_options *opts) {
  git_commit *parent = nullptr;
  git_tree *a = nullptr, *b = nullptr;
  bool result = false;
  if (!git_commit_parent(&parent, commit, index) &&
      !git_commit_tree(&a, parent) && !git_commit_tree(&b, commit)) {
    // notify() aborts at the first delta, so a failed diff means a change.
    git_diff *diff = nullptr;
    result =
        git_diff_tree_to_tree(&diff, git_commit_owner(commit), a, b, opts) != 0;
    git_diff_free(diff);
  }

  git_tree_free(a);
  git_tree_free(b);
  git_commit_free(parent);
  return result;
}

// Same rule as libgit2's revwalk pathspec: roots containing the path, and
// other commits (merges included) that differ from every parent on it.
bool touches(git_commit *commit, const git_diff_options *opts) {
  unsigned int count = git_commit_parentcount(commit);
  if (count == 0) {
    git_pathspec *pathspec = nullptr;
    git_pathspec_new(&pathspec, &opts->pathspec);

    int flags = GIT_PATHSPEC_NO_MATCH_ERROR;
    if (opts->flags & GIT_DIFF_DISABLE_PATHSPEC_MATCH)
      flags |= GIT_PATHSPEC_NO_GLOB;

    git_tree *tree = nullptr;
    git_commit_tree(&tree, commit);
    bool match = !git_pathspec_match_tree(nullptr, tree, flags, pathspec);

    git_tree_free(tree);
    git_pathspec_free(pathspec);
    return match;
  }

  for (unsigned int i = 0; i < count; ++i) {
    if (!differs(commit, i, opts))
      return false;
  }

  return true;
}

} // namespace
#endif

RevWalk::RevWalk() {}

RevWalk::RevWalk(git_revwalk *walker) : d(std::make_shared<Data>(walker)) {}

git_revwalk *RevWalk::walk() const { return d ? d->walk : nullptr; }

bool RevWalk::hide(const Commit &commit) {
  return !git_revwalk_hide(walk(), commit);
}

bool RevWalk::hide(const Reference &ref) {
  Commit commit = ref.target();
  return commit.isValid() ? hide(commit) : false;
}

bool RevWalk::push(const Commit &commit) {
  return !git_revwalk_push(walk(), commit);
}

bool RevWalk::push(const Reference &ref) {
  Commit commit = ref.target();
  return commit.isValid() ? push(commit) : false;
}

bool RevWalk::setPathspec(const QString &pathspec) {
  if (!d)
    return false;

#if LIBGIT2_VERSION_CHECK(2, 0, 0)
  QByteArray buffer = pathspec.toUtf8();
  char *data = buffer.data();
  git_strarray paths = {&data, 1};

  git_pathspec *ps = nullptr;
  if (git_pathspec_new(&ps, &paths) || git_revwalk_pathspec(walk(), ps)) {
    git_pathspec_free(ps);
    return false;
  }

  git_pathspec_free(d->pathspec);
  d->pathspec = ps;
#else
  d->path = pathspec;
#endif
  return true;
}

Commit RevWalk::next() const {
#if !LIBGIT2_VERSION_CHECK(2, 0, 0)
  // libgit2 only filters the walk itself from 2.0, so filter here.
  QString path = d ? d->path : QString();
  QByteArray buffer = path.toUtf8();
  char *data = buffer.data();

  git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
  diffopts.notify_cb = notify;
  diffopts.pathspec.count = 1;
  diffopts.pathspec.strings = &data;
  if (!path.contains(QRegularExpression("[*?]")))
    diffopts.flags |= GIT_DIFF_DISABLE_PATHSPEC_MATCH;
#endif

  Id id;
  while ((id = nextID()) && id.isValid()) {
    git_commit *commit = nullptr;
    if (git_commit_lookup(&commit, git_revwalk_repository(walk()), id)) {
      return Commit();
    }

#if !LIBGIT2_VERSION_CHECK(2, 0, 0)
    if (!path.isEmpty() && !touches(commit, &diffopts)) {
      git_commit_free(commit);
      continue;
    }
#endif

    return Commit(commit);
  }
  return Commit();
}

Id RevWalk::nextID() const {
  git_oid id;
  int error;
  while (!(error = git_revwalk_next(&id, walk()))) {
    return Id(id);
  }
  return Id();
}

} // namespace git
