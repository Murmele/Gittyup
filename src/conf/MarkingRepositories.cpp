#include "MarkingRepositories.h"
#include "MarkingRepository.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>

MarkingGroup::MarkingGroup(const QString &name, QObject *parent)
    : QObject(parent), mName(name) {}

QString MarkingGroup::name() const { return mName; }
int MarkingGroup::count() const { return mRepos.size(); }
MarkingRepository *MarkingGroup::repository(int index) const {
  return mRepos.at(index);
}
QString MarkingGroup::displayName(int index) const {
  MarkingRepository *repo = repository(index);
  QString name = QFileInfo(repo->gitpath()).fileName();
  int sections = 1;
  for (int i = 0; i < mRepos.size(); ++i) {
    if (i == index)
      continue;
    QString other = QFileInfo(mRepos.at(i)->gitpath()).fileName();
    if (other == name)
      ++sections;
  }
  if (sections > 1)
    name = repo->gitpath().section('/', -sections, -1);
  return name;
}
bool MarkingGroup::contains(const QString &path) const {
  for (MarkingRepository *repo : mRepos)
    if (repo->gitpath() == path)
      return true;
  return false;
}
void MarkingGroup::add(const QString &path) {
  if (!contains(path))
    mRepos.append(new MarkingRepository(path, this));
}
void MarkingGroup::remove(int index) { delete mRepos.takeAt(index); }

MarkingRepositories::MarkingRepositories() { load(); }

int MarkingRepositories::groupCount() const { return mGroups.size(); }
MarkingGroup *MarkingRepositories::group(int index) const {
  return mGroups.at(index);
}

MarkingGroup *MarkingRepositories::groupContaining(const QString &path) const {
  for (MarkingGroup *group : mGroups)
    if (group->contains(path))
      return group;
  return nullptr;
}

QString MarkingRepositories::displayName(const QString &path,
                                         const QString &name) const {
  MarkingGroup *group = groupContaining(path);
  if (!group)
    return name;
  return QString("%1:%2").arg(group->name(), name);
}

MarkingGroup *MarkingRepositories::addGroup(const QString &name) {
  for (MarkingGroup *group : mGroups)
    if (group->name() == name)
      return group;
  emit aboutToChange();
  MarkingGroup *group = new MarkingGroup(name, this);
  mGroups.append(group);
  store();
  emit changed();
  return group;
}

void MarkingRepositories::mark(const QString &path, int groupIndex) {
  if (groupIndex < 0 || groupIndex >= mGroups.size())
    return;

  MarkingGroup *target = mGroups.at(groupIndex);
  MarkingGroup *current = groupContaining(path);
  if (current == target)
    return;

  emit aboutToChange();
  if (current) {
    for (int i = 0; i < current->count(); ++i) {
      if (current->repository(i)->gitpath() == path) {
        current->remove(i);
        break;
      }
    }
    if (current->count() == 0)
      delete mGroups.takeAt(mGroups.indexOf(current));
  }
  target->add(path);
  store();
  emit changed();
}

void MarkingRepositories::remove(int groupIndex, int repositoryIndex) {
  if (groupIndex < 0 || groupIndex >= mGroups.size() ||
      repositoryIndex < 0 || repositoryIndex >= mGroups.at(groupIndex)->count())
    return;
  emit aboutToChange();
  mGroups.at(groupIndex)->remove(repositoryIndex);
  if (mGroups.at(groupIndex)->count() == 0)
    delete mGroups.takeAt(groupIndex);
  store();
  emit changed();
}

void MarkingRepositories::removeGroup(int groupIndex) {
  if (groupIndex < 0 || groupIndex >= mGroups.size())
    return;
  emit aboutToChange();
  delete mGroups.takeAt(groupIndex);
  store();
  emit changed();
}

MarkingRepositories *MarkingRepositories::instance() {
  static MarkingRepositories *instance = nullptr;
  if (!instance)
    instance = new MarkingRepositories;
  return instance;
}

void MarkingRepositories::store() const {
  QSettings settings;
  settings.beginGroup("marking");
  settings.remove("");
  QStringList names;
  for (MarkingGroup *group : mGroups) {
    names.append(group->name());
    settings.beginGroup(group->name());
    QStringList paths;
    for (int i = 0; i < group->count(); ++i)
      paths.append(group->repository(i)->gitpath());
    settings.setValue("repositories", paths);
    settings.endGroup();
  }
  settings.setValue("groups", names);
  settings.endGroup();
}

void MarkingRepositories::load() {
  QSettings settings;
  settings.beginGroup("marking");
  for (const QString &name : settings.value("groups").toStringList()) {
    MarkingGroup *group = new MarkingGroup(name, this);
    for (const QString &path : settings.value(name + "/repositories").toStringList())
      group->add(path);
    mGroups.append(group);
  }
  settings.endGroup();
}
