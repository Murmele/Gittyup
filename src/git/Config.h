//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CONFIG_H
#define CONFIG_H

#include "Repository.h"
#include "git2/config.h"
#include <QSharedPointer>

namespace git {

class Config {
public:
  class Iterator;

  class Entry {
  public:
    bool isValid() const { return !d.isNull(); }
    explicit operator bool() const { return isValid(); }

    QString name() const;

    template <typename T> T value() const;

  private:
    Entry(git_config_entry *entry = nullptr, bool owned = false);

    QSharedPointer<git_config_entry> d;

    friend class Iterator;
  };

  class Iterator {
  public:
    bool isValid() const { return !d.isNull(); }

    Entry next() const;

  private:
    Iterator(git_config_iterator *iterator = nullptr);

    QSharedPointer<git_config_iterator> d;

    friend class Config;
  };

  bool isValid() const { return !d.isNull(); }

  bool addFile(const QString &path,
               git_config_level_t level = GIT_CONFIG_LEVEL_APP,
               const Repository &repo = git::Repository());

  template <typename T>
  T value(const QString &key, const T &defaultValue = T()) const;

  template <typename T> void setValue(const QString &key, const T &value);

  bool remove(const QString &key);

  QStringList value(const QString &key, const QString &regexp,
                    const QStringList &defaultValue) const;
  void setValue(const QString &key, const QString regexp, const QString &value);
  bool remove(const QString &key, const QString regexp);

  int removeBackendEntries(const QString &key);

  Iterator glob(const QString &pattern) const;

  static Config global();
  static QString globalPath();

  static Config appGlobal();

  static Config open(const QString &path);

private:
  Config(git_config *config = nullptr);

  QSharedPointer<git_config> d;

  friend class Repository;
};

// entry specializations
template <> bool Config::Entry::value<bool>() const;
template <> int Config::Entry::value<int>() const;
template <> QString Config::Entry::value<QString>() const;

template <typename T> T Config::Entry::value() const {
  static_assert(sizeof(T) == 0, "no specialization found");
  return T();
}

// config specializations
template <> bool Config::value<bool>(const QString &, const bool &) const;
template <> void Config::setValue<bool>(const QString &, const bool &);

template <> int Config::value<int>(const QString &, const int &) const;
template <> void Config::setValue<int>(const QString &, const int &);

template <>
QString Config::value<QString>(const QString &, const QString &) const;
template <> void Config::setValue<QString>(const QString &, const QString &);

template <typename T>
T Config::value(const QString &key, const T &defaultValue) const {
  static_assert(sizeof(T) == 0, "no specialization found. Please implement "
                                "specialization for this type!");
  return T();
}

template <typename T>
void Config::setValue(const QString &key, const T &value) {
  static_assert(sizeof(T) == 0, "no specialization found. Please implement "
                                "specialization for this type!");
}

} // namespace git

#endif
