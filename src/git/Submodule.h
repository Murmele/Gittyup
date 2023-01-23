//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SUBMODULE_H
#define SUBMODULE_H

#include "Remote.h"
#include "Result.h"
#include "git2/submodule.h"
#include <QSharedPointer>

namespace git {

class Id;
class Repository;

class Submodule {
public:
  Submodule();

  bool isValid() const { return !d.isNull(); }
  explicit operator bool() const { return isValid(); }

  bool isInitialized() const;
  void initialize() const;
  void deinitialize() const;

  QString name() const;
  QString path() const;

  QString url() const;
  void setUrl(const QString &url);

  QString branch() const;
  void setBranch(const QString &branch);

  Id headId() const;
  Id indexId() const;
  Id workdirId() const;

  /*!
   * \brief status
   * Return the status of the submodule as a combination of flags described in
   * https://libgit2.org/libgit2/index.html#HEAD/type/git_submodule_status_t
   * \return current status
   */
  int status() const;

  Result update(Remote::Callbacks *callbacks, bool init = false,
                bool checkout_force = false);

  Repository open() const;

private:
  Submodule(git_submodule *submodule);
  operator git_submodule *() const;

  QSharedPointer<git_submodule> d;

  friend class Index;
  friend class Repository;
};

} // namespace git

Q_DECLARE_METATYPE(git::Submodule);

#endif
