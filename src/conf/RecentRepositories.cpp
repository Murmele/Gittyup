//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "RecentRepositories.h"
#include "RecentRepository.h"
#include "util/Path.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>

namespace {

const QString kRecentKey = "recent";
const QString kFilterKey = "recent/filter";

} // namespace

RecentRepositories::RecentRepositories(QObject *parent) : QObject(parent) {
  load();
}

int RecentRepositories::count() const { return mRepos.size(); }

RecentRepository *RecentRepositories::repository(int index) const {
  return mRepos.at(index);
}

void RecentRepositories::clear() {
  emit repositoryAboutToBeRemoved();

  qDeleteAll(mRepos);
  mRepos.clear();
  QSettings().remove(kRecentKey);

  emit repositoryRemoved();
}

void RecentRepositories::remove(int index) {
  emit repositoryAboutToBeRemoved();

  delete mRepos.takeAt(index);
  store();

  emit repositoryRemoved();
}

void RecentRepositories::add(QString path) {
  emit repositoryAboutToBeAdded();

  auto end = mRepos.end();
  RecentRepository *repo = new RecentRepository(path, this);
  auto it = std::remove_if(mRepos.begin(), end, [repo](RecentRepository *rhs) {
#ifdef Q_OS_WIN
    return repo->path().compare(rhs->path(), Qt::CaseInsensitive) == 0;
#else
    return (repo->path() == rhs->path());
#endif
  });

  if (it != end)
    mRepos.erase(it, end);

  mRepos.prepend(repo);
  store();

  emit repositoryAdded();
}

RecentRepositories *RecentRepositories::instance() {
  static RecentRepositories *instance = nullptr;
  if (!instance)
    instance = new RecentRepositories(qApp);
  return instance;
}

void RecentRepositories::store() {
  QStringList paths;
  foreach (RecentRepository *repo, mRepos)
    paths.append(repo->path());

  QSettings().setValue(kRecentKey, paths);

  // Reload repos.
  load();
}

void RecentRepositories::load() {
  QSettings settings;
  QStringList paths = settings.value(kRecentKey).toStringList();
  paths.removeDuplicates();

  // Filter out paths that no longer exist.
  if (settings.value(kFilterKey, true).toBool()) {
    auto end = paths.end();
    auto it = std::remove_if(paths.begin(), end, [](const QString &path) {
      return !QFileInfo(path).exists();
    });

    if (it != end)
      paths.erase(it, end);

#ifdef Q_OS_WIN
    for (QString &path : paths) {
      path = util::canonicalizePath(path);
    }

    paths.removeDuplicates();
#endif
  }

  // Store filtered list.
  if (!paths.isEmpty()) {
    settings.setValue(kRecentKey, paths);
  } else {
    settings.remove(kRecentKey);
  }

  // Reset repos.
  qDeleteAll(mRepos);
  mRepos.clear();

  foreach (const QString &path, paths) {
    RecentRepository *repo = new RecentRepository(path, this);
    auto functor = [repo](RecentRepository *rhs) {
      return (repo->name() == rhs->name());
    };

    QList<RecentRepository *>::iterator it, end = mRepos.end();
    while ((it = std::find_if(mRepos.begin(), end, functor)) != end) {
      (*it)->increment();
      repo->increment();
    }

    mRepos.append(repo);
  }
}
