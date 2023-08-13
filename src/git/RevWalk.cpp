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
#include <QRegularExpression>
#include "trace.h"

namespace git {

namespace {

int notify(const git_diff *, const git_diff_delta *, const char *, void *) {
  // Abort the diff.
  return -1;
}

} // namespace

RevWalk::RevWalk() {}

RevWalk::RevWalk(git_revwalk *walker) : d(walker, git_revwalk_free) {}

bool RevWalk::hide(const Commit &commit) {
  return !git_revwalk_hide(d.data(), commit);
}

bool RevWalk::hide(const Reference &ref) {
  Commit commit = ref.target();
  return commit.isValid() ? hide(commit) : false;
}

bool RevWalk::push(const Commit &commit) {
  return !git_revwalk_push(d.data(), commit);
}

bool RevWalk::push(const Reference &ref) {
  Commit commit = ref.target();
  return commit.isValid() ? push(commit) : false;
}

Commit RevWalk::next(const QString &path) const {
  PERFTRACE("Path:" + path);
  git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
  diffopts.notify_cb = notify;

  // Convert pathspec to UTF-8.
  QByteArray buffer = path.toUtf8();
  char *data = buffer.data();
  if (!path.isEmpty()) {
    diffopts.pathspec.count = 1;
    diffopts.pathspec.strings = &data;
    if (!path.contains(QRegularExpression("[*?]")))
      diffopts.flags |= GIT_DIFF_DISABLE_PATHSPEC_MATCH;
  }

  git_oid id;
  int counter = 0;
  while (1) {
    PERFTRACE("while !git_revwalk_next(). d valid: " + QString::number((int)!d.isNull()));
    if (git_revwalk_next(&id, d.data()))
      break;
    counter ++;
    git_commit *commit = nullptr;
    git_commit_lookup(&commit, git_revwalk_repository(d.data()), &id);
    Q_ASSERT(commit);

    if (path.isEmpty())
      return Commit(commit);

    switch (git_commit_parentcount(commit)) {
      case 0: {
        git_pathspec *pathspec = nullptr;
        git_pathspec_new(&pathspec, &diffopts.pathspec);

        int flags = GIT_PATHSPEC_NO_MATCH_ERROR;
        if (diffopts.flags & GIT_DIFF_DISABLE_PATHSPEC_MATCH)
          flags |= GIT_PATHSPEC_NO_GLOB;

        git_tree *tree;
        git_commit_tree(&tree, commit);
        bool filter = git_pathspec_match_tree(nullptr, tree, flags, pathspec);

        git_tree_free(tree);
        git_pathspec_free(pathspec);

        if (!filter)
          return Commit(commit);

        break;
      }

      case 1: {
        git_commit *parent;
        git_commit_parent(&parent, commit, 0);

        git_tree *a, *b;
        git_commit_tree(&a, parent);
        git_commit_tree(&b, commit);

        git_diff *diff;
        git_repository *repo = git_commit_owner(commit);
        bool filter = !git_diff_tree_to_tree(&diff, repo, a, b, &diffopts);

        git_diff_free(diff);
        git_tree_free(a);
        git_tree_free(b);
        git_commit_free(parent);

        if (!filter)
          return Commit(commit);

        break;
      }

      default:
        // Filter merges.
        break;
    }

    git_commit_free(commit);
  }

  return Commit();
}

} // namespace git
