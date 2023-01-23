//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Location.h"
#include "git/Commit.h"
#include "git/Repository.h"
#include "git/Signature.h"

Location::Location() {}

Location::Location(RepoView::ViewMode mode, const QString &ref,
                   const QString &id, const QString &file)
    : mMode(mode), mRef(ref), mId(id), mFile(file), mValid(true) {}

QString Location::toString(const git::Repository &repo) const {
  QString fmt = tr("%1 | %2");
  git::Commit commit = repo.lookupCommit(mId);
  if (!commit.isValid())
    return fmt.arg(tr("NC"), tr("Not Committed"));

  QString initials = commit.author().initials();
  return fmt.arg(initials, commit.summary(git::Commit::SubstituteEmoji));
}

bool Location::operator==(const Location &rhs) const {
  return (mMode == rhs.mode() && mRef == rhs.ref() && mId == rhs.id() &&
          mFile == rhs.file());
}
