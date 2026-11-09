#ifndef MARKINGREPOSITORIES_H
#define MARKINGREPOSITORIES_H

#include "RecentRepositories.h"

class MarkingRepository;

class MarkingGroup : public QObject {
  Q_OBJECT
public:
  explicit MarkingGroup(const QString &name, QObject *parent = nullptr);
  QString name() const;
  int count() const;
  MarkingRepository *repository(int index) const;
  QString displayName(int index) const;
  bool contains(const QString &path) const;
  void add(const QString &path);
  void remove(int index);

private:
  QString mName;
  QList<MarkingRepository *> mRepos;
};

Q_DECLARE_METATYPE(MarkingGroup *)

class MarkingRepositories : public QObject {
  Q_OBJECT
public:
  MarkingRepositories();

  int groupCount() const;
  MarkingGroup *group(int index) const;
  MarkingGroup *groupContaining(const QString &path) const;
  QString displayName(const QString &path, const QString &name) const;
  MarkingGroup *addGroup(const QString &name);
  void mark(const QString &path, int groupIndex);
  void remove(int groupIndex, int repositoryIndex);
  void removeGroup(int groupIndex);

  static MarkingRepositories *instance();

signals:
  // Emitted before any marked repository or group is deleted, so views can
  // drop indexes that point at those objects.
  void aboutToChange();
  void changed();

private:
  void load();
  void store() const;
  QList<MarkingGroup *> mGroups;
};

#endif // MARKINGREPOSITORIES_H
