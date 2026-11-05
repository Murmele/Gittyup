//
//          Copyright (c) 2026, Gittyup
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Anand Hegde
//

#ifndef COMMITSIGNER_H
#define COMMITSIGNER_H

#include "git2/types.h"
#include <QByteArray>
#include <QString>

namespace git {

// Creates commit objects, signing them the way git does when commit.gpgsign
// is set. The signature is made by running the external program configured
// by gpg.format / gpg.program / gpg.<format>.program with the key from
// user.signingkey, so private keys never pass through Gittyup.
class CommitSigner {
public:
  CommitSigner(git_repository *repo);

  // True if commit.gpgsign is set for the repository.
  bool isEnabled() const { return mEnabled; }

  // Create a signed commit object without updating any reference. Returns 0
  // on success, otherwise an error code with the libgit2 error message set.
  int createSignedCommit(git_oid *out, const git_signature *author,
                         const git_signature *committer, const char *encoding,
                         const char *message, const git_tree *tree,
                         size_t parentCount, const git_commit *parents[]) const;

  // Returns the output of the last failed signing attempt, then clears it.
  static QString takeLastError();

private:
  bool sign(const QByteArray &content, const git_signature *committer,
            QByteArray &signature, QString &error) const;
  bool signOpenPgp(const QByteArray &content, const QString &key,
                   QByteArray &signature, QString &error) const;
  bool signSsh(const QByteArray &content, QByteArray &signature,
               QString &error) const;

  git_repository *mRepo;
  bool mEnabled = false;
  QString mFormat;
  QString mProgram;
  QString mKey;
};

} // namespace git

#endif
