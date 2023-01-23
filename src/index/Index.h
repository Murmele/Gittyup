//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef INDEX_H
#define INDEX_H

#include "git/Id.h"
#include "git/Repository.h"
#include <QList>
#include <QObject>
#include <QVector>
#include <functional>

namespace git {
class Commit;
}

class Index : public QObject {
  Q_OBJECT

public:
  enum Field {
    // fields
    Id,
    Author,
    Email,
    Message,
    Date,
    Path,
    File,
    Scope,
    Context,
    Addition,
    Deletion,
    Any,

    // subfields
    Comment = 1 << 4,
    String = 2 << 4,
    Identifier = 3 << 4,

    // pseudo-fields
    Is,
    Before,
    After,
    Pathspec
  };

  struct Word {
    Word(const QByteArray &key, quint32 value = 0) : key(key), value(value) {}

    bool operator<(const Word &rhs) const { return key < rhs.key; }

    QByteArray key;
    quint32 value;
  };

  struct Term {
    Term(Field field, const QString &text) : field(field), text(text) {}

    Field field;
    QString text;
  };

  struct Posting {
    quint32 id;
    quint8 field;
    QVector<quint32> positions;
  };

  using IdList = QList<git::Id>;
  using Dictionary = QList<Word>;
  using PostingMap = QMap<QByteArray, QVector<Index::Posting>>;
  using Predicate = std::function<bool(const QByteArray &)>;

  Index(const git::Repository &repo, QObject *parent = nullptr);

  bool isValid() const;

  git::Repository repo() const { return mRepo; }
  IdList &ids() { return mIds; }
  Dictionary &dict() { return mDict; }

  void reset();
  void clean();
  bool remove();
  bool write(PostingMap map);

  QList<git::Commit> commits(const QString &filter) const;
  QList<git::Commit> commits(const QList<Posting> &postings) const;

  QList<Posting> postings(const Term &term, bool positional = false) const;
  QList<Posting> postings(const Predicate &pred, Field field = Any) const;

  QMap<Field, QStringList> fieldMap(const QString &prefix = QString()) const;

  // constants
  static quint8 version();
  static int staleLockTime();
  static QString dateFormat();

  // Get the canonical name for the given field.
  static QByteArray fieldName(Field field);

  // Get the index directory for the given repository.
  static QDir indexDir(const git::Repository &repo);
  static QString lockFile(const git::Repository &repo);

  // vint
  static quint32 readVInt(QDataStream &in);
  static void writeVInt(QDataStream &out, quint32 arg);

  // positions
  static void readPositions(QDataStream &in, QVector<quint32> &positions);
  static void writePositions(QDataStream &out,
                             const QVector<quint32> &positions);

  // Enable logging. The log is written to the index dir.
  static bool isLoggingEnabled();
  static void setLoggingEnabled(bool enable);

signals:
  void indexReset();

private:
  // index version
  quint8 readVersion() const;
  void writeVersion() const;

  QDir indexDir() const;

  git::Repository mRepo;
  IdList mIds;
  Dictionary mDict;

  static bool sLoggingEnabled;
};

#endif
