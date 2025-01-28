//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Index.h"
#include "GenericLexer.h"
#include "LPegLexer.h"
#include "Query.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Diff.h"
#include "git/Patch.h"
#include "git/Reference.h"
#include "git/RevWalk.h"
#include "git/Signature.h"
#include <QLockFile>
#include <QSettings>
#include <QtConcurrent>

#include <stdexcept>

namespace {

const QString kLogKey = "debug/indexer";

const QString kIndexDir = "index";
const QString kIdFile = "ids";
const QString kDictFile = "dict";
const QString kPostFile = "post";
const QString kProxFile = "prox";
const QString kLockFile = "lock";
const QString kVersionFile = "version";

const QStringList kIndexFiles = {kIdFile, kDictFile, kPostFile, kProxFile};

} // namespace

bool Index::sLoggingEnabled = false;

Index::Index(const git::Repository &repo, QObject *parent)
    : QObject(parent), mRepo(repo) {
  // Read log setting.
  sLoggingEnabled = QSettings().value(kLogKey).toBool();

  // Clean up temporary files.
  clean();

  // Check version.
  if (readVersion() < version())
    remove();

  // Read data from disk.
  reset();
}

bool Index::isValid() const { return indexDir().exists(kIdFile); }

void Index::reset() {
  mIds.clear();
  mDict.clear();

  // Read already indexed ids.
  QDir dir = indexDir();
  QFile idFile(dir.filePath(kIdFile));
  if (idFile.open(QIODevice::ReadOnly)) {
    while (idFile.bytesAvailable() > 0)
      mIds.append(idFile.read(GIT_OID_SHA1));
  }

  // Read dictionary.
  QFile dictFile(dir.filePath(kDictFile));
  if (dictFile.open(QIODevice::ReadOnly)) {
    QDataStream dictIn(&dictFile);
    while (!dictIn.atEnd()) {
      quint32 pos;
      QByteArray word;
      dictIn >> word >> pos;
      mDict.append(Word(word, pos));
    }
  }

  emit indexReset();
}

void Index::clean() {
  QStringList filters;
  foreach (const QString &file, kIndexFiles)
    filters.append(file + ".*");

  QDir dir = indexDir();
  QStringList files = dir.entryList(filters, QDir::Files);
  if (files.isEmpty())
    return;

  // Try to lock the index for writing.
  QLockFile lock(lockFile(mRepo));
  lock.setStaleLockTime(staleLockTime());
  if (!lock.tryLock())
    return;

  foreach (const QString &file, files)
    dir.remove(file);
}

bool Index::remove() {
  // Try to lock the index for writing.
  QLockFile lock(lockFile(mRepo));
  lock.setStaleLockTime(staleLockTime());
  if (!lock.tryLock())
    return false;

  QDir dir = indexDir();
  foreach (const QString &file, kIndexFiles) {
    if (!dir.remove(file))
      return false;
  }

  reset();
  return true;
}

bool Index::write(PostingMap map) {
  if (map.isEmpty())
    return false;

  // Open files for writing.
  QDir dir = indexDir();
  QSaveFile idFile(dir.filePath(kIdFile));
  QSaveFile postFile(dir.filePath(kPostFile));
  QSaveFile proxFile(dir.filePath(kProxFile));
  QSaveFile dictFile(dir.filePath(kDictFile));
  if (!idFile.open(QIODevice::WriteOnly) ||
      !postFile.open(QIODevice::WriteOnly) ||
      !proxFile.open(QIODevice::WriteOnly) ||
      !dictFile.open(QIODevice::WriteOnly))
    return false;

  // Write id file.
  foreach (const git::Id &id, mIds)
    idFile.write(id.toByteArray(), GIT_OID_SHA1);

  // Merge new entries into existing postings file.
  // Write dictionary and postings files in lockstep.
  QDataStream postOut(&postFile);
  QDataStream proxOut(&proxFile);
  QDataStream dictOut(&dictFile);

  // Open existing postings file.
  QDataStream postIn;
  QFile postInFile(dir.filePath(kPostFile));
  if (postInFile.open(QIODevice::ReadOnly))
    postIn.setDevice(&postInFile);

  QDataStream proxIn;
  QFile proxInFile(dir.filePath(kProxFile));
  if (proxInFile.open(QIODevice::ReadOnly))
    proxIn.setDevice(&proxInFile);

  // Create a copy of the existing dictionary.
  Dictionary dict = mDict;
  mDict.clear();

  // Iterate over new and existing entries simultaneously.
  PostingMap::const_iterator newEnd = map.end();
  PostingMap::const_iterator newIt = map.begin();
  Dictionary::const_iterator end = dict.end();
  Dictionary::const_iterator it = dict.begin();
  while (newIt != newEnd || it != end) {
    QByteArray key;
    QVector<Posting> postings;
    if (it == end || (newIt != newEnd && newIt.key() < it->key)) {
      // Add new entry.
      key = newIt.key();
      postings = newIt.value();
      ++newIt;
    } else {
      // Read existing postings.
      key = it->key;
      quint32 postCount = readVInt(postIn);
      bool end = (newIt == newEnd || newIt.key() != it->key);
      postings.reserve(postCount + (!end ? newIt.value().size() : 0));
      for (quint32 i = 0; i < postCount; ++i) {
        quint32 proxPos;
        Posting posting;
        posting.id = readVInt(postIn);
        postIn >> posting.field >> proxPos;
        readPositions(proxIn, posting.positions);
        postings.append(posting);
      }

      // Merge in new entries.
      if (!end) {
        postings.append(newIt.value());
        ++newIt;
      }

      ++it;
    }

    // Write dictionary.
    quint32 postPos = postFile.pos(); // truncate
    mDict.append(Word(key, postPos));
    dictOut << key << postPos;

    // Write postings.
    writeVInt(postOut, postings.size());
    foreach (const Posting &posting, postings) {
      quint32 proxPos = proxFile.pos(); // truncate
      writeVInt(postOut, posting.id);
      postOut << posting.field << proxPos;
      writePositions(proxOut, posting.positions);
    }
  }

  // Close in files.
  postInFile.close();
  proxInFile.close();

  // Write out files.
  postFile.commit();
  proxFile.commit();
  dictFile.commit();
  idFile.commit();

  // Write version last.
  writeVersion();

  return true;
}

QList<git::Commit> Index::commits(const QString &filter) const {
  if (filter.isEmpty())
    return QList<git::Commit>();

  // Parse query.
  QueryRef query = Query::parseQuery(filter);
  if (!query)
    return QList<git::Commit>();

  // Sort by commit date.
  QList<git::Commit> commits = query->commits(this);
  std::sort(commits.begin(), commits.end(),
            [](const git::Commit &lhs, const git::Commit &rhs) {
              return (lhs.committer().date() > rhs.committer().date());
            });

  return commits;
}

QList<git::Commit> Index::commits(const QList<Posting> &postings) const {
  // Look up commits.
  QSet<git::Commit> commits;
  foreach (const Posting &posting, postings) {
    // FIXME: Remove deleted commits on write.
    if (git::Commit commit = mRepo.lookupCommit(mIds.at(posting.id)))
      commits.insert(commit);
  }

  return commits.values();
}

QList<Index::Posting> Index::postings(const Term &term, bool positional) const {
  Word word(term.text.toLower().toUtf8());
  Dictionary::const_iterator end = mDict.end();
  Dictionary::const_iterator it = std::lower_bound(mDict.begin(), end, word);
  if (it == end || it->key != word.key)
    return QList<Posting>();

  QDir dir = indexDir();
  QFile file(dir.filePath(kPostFile));
  if (!file.open(QIODevice::ReadOnly))
    return QList<Posting>();

  QFile proxFile;
  if (positional) {
    proxFile.setFileName(dir.filePath(kProxFile));
    if (!proxFile.open(QIODevice::ReadOnly))
      return QList<Posting>();
  }

  // Skip to the correct entry.
  file.seek(it->value);
  QDataStream in(&file);

  // Read list.
  QList<Posting> postings;
  quint32 postCount = readVInt(in);
  for (quint32 i = 0; i < postCount; ++i) {
    quint32 proxPos;
    Posting posting;
    posting.id = readVInt(in);
    in >> posting.field >> proxPos;

    // Filter by field.
    quint8 field = posting.field & 0x0F;
    quint8 subfield = posting.field & 0xF0;
    if (term.field == Any || term.field == field || term.field == subfield) {
      // Load positions when needed.
      if (positional) {
        proxFile.seek(proxPos);
        QDataStream proxIn(&proxFile);
        readPositions(proxIn, posting.positions);
      }

      postings.append(posting);
    }
  }

  return postings;
}

QList<Index::Posting> Index::postings(const Predicate &pred,
                                      Field field) const {
  QDir dir = indexDir();
  QFile file(dir.filePath(kPostFile));
  if (!file.open(QIODevice::ReadOnly))
    return QList<Posting>();

  QList<Posting> postings;
  foreach (const Word &word, mDict) {
    // Test predicate.
    if (!pred(word.key))
      continue;

    // Skip to the correct entry.
    file.seek(word.value);
    QDataStream in(&file);

    // Read list.
    quint32 postCount = readVInt(in);
    for (quint32 i = 0; i < postCount; ++i) {
      quint32 proxPos;
      Posting posting;
      posting.id = readVInt(in);
      in >> posting.field >> proxPos;

      // Match field.
      quint8 postField = posting.field & 0x0F;
      quint8 postSubfield = posting.field & 0xF0;
      if (field == Any || field == postField || field == postSubfield)
        postings.append(posting);
    }
  }

  return postings;
}

QMap<Index::Field, QStringList> Index::fieldMap(const QString &prefix) const {
  QDir dir = indexDir();
  QFile file(dir.filePath(kPostFile));
  if (!file.open(QIODevice::ReadOnly))
    return QMap<Field, QStringList>();

  Dictionary::const_iterator it = mDict.constBegin();
  Dictionary::const_iterator end = mDict.constEnd();
  if (!prefix.isEmpty()) {
    Word word(prefix.toLower().toUtf8());
    it = std::lower_bound(it, end, word);
    end = std::find_if(it, end, [word](const Word &arg) {
      return !arg.key.startsWith(word.key);
    });
  }

  QMap<Field, QStringList> map;
  while (it != end) {
    // Skip to the correct entry.
    file.seek(it->value);
    QDataStream in(&file);

    // Read list.
    QString name = it->key;
    quint32 postCount = readVInt(in);
    for (quint32 i = 0; i < postCount; ++i) {
      quint8 field;
      quint32 proxPos;
      readVInt(in); // Discard id.
      in >> field >> proxPos;

      QStringList &fieldList = map[static_cast<Field>(field & 0x0F)];
      if (fieldList.isEmpty() || fieldList.last() != name)
        fieldList.append(name);

      if (quint8 subfield = field & 0xF0) {
        QStringList &subfieldList = map[static_cast<Field>(subfield)];
        if (subfieldList.isEmpty() || subfieldList.last() != name)
          subfieldList.append(name);
      }
    }

    ++it;
  }

  return map;
}

quint8 Index::version() { return 2; }

int Index::staleLockTime() { return 24 * 60 * 60 * 1000; }

QString Index::dateFormat() { return "yyyy/MM/dd"; }

QByteArray Index::fieldName(Index::Field field) {
  switch (field) {
    case Index::Id:
      return "id";
    case Index::Author:
      return "author";
    case Index::Email:
      return "email";
    case Index::Message:
      return "message";
    case Index::Date:
      return "date";
    case Index::Path:
      return "path";
    case Index::File:
      return "file";
    case Index::Scope:
      return "scope";
    case Index::Context:
      return "context";
    case Index::Addition:
      return "addition";
    case Index::Deletion:
      return "deletion";
    case Index::Any:
      return "any";
    case Index::Comment:
      return "comment";
    case Index::String:
      return "string";
    case Index::Identifier:
      return "identifier";
    case Index::Is:
      return "is";
    case Index::Before:
      return "before";
    case Index::After:
      return "after";
    case Index::Pathspec:
      return "pathspec";
  }
  throw std::runtime_error("unreachable; value=" +
                           std::to_string(static_cast<int>(field)));
}

QDir Index::indexDir(const git::Repository &repo) {
  QDir dir = repo.appDir();
  dir.mkpath(kIndexDir);
  dir.cd(kIndexDir);
  return dir;
}

QString Index::lockFile(const git::Repository &repo) {
  return indexDir(repo).filePath(kLockFile);
}

quint8 Index::readVersion() const {
  QFile file(indexDir().filePath(kVersionFile));
  if (!file.open(QFile::ReadOnly))
    return 0;

  quint8 version = 0;
  QDataStream(&file) >> version;
  return version;
}

void Index::writeVersion() const {
  QFile file(indexDir().filePath(kVersionFile));
  if (file.open(QFile::WriteOnly))
    QDataStream(&file) << version();
}

// Use the same variable length VInt encoding as Lucene.
quint32 Index::readVInt(QDataStream &in) {
  // Read least significant byte.
  quint8 byte;
  in >> byte;
  quint32 result = byte & 0x7F;

  // Continue reading more significant bytes.
  for (quint32 shift = 7; byte & 0x80; shift += 7) {
    in >> byte;
    result |= (byte & 0x7F) << shift;
  }

  return result;
}

void Index::writeVInt(QDataStream &out, quint32 arg) {
  // Write less significant bytes first.
  // Most significant bit is set.
  while (arg & ~0x7F) {
    out << static_cast<quint8>((arg & 0x7F) | 0x80);
    arg >>= 7;
  }

  // Write most significant byte last.
  // Most significant bit is unset.
  out << static_cast<quint8>(arg);
}

// Write deltas to minimize bytes per position.
void Index::readPositions(QDataStream &in, QVector<quint32> &positions) {
  quint32 prev = 0;
  quint32 count = readVInt(in);
  positions.reserve(count);
  for (quint32 i = 0; i < count; ++i) {
    // Convert to absolute from delta.
    quint32 position = prev + readVInt(in);
    positions.append(position);
    prev = position;
  }
}

void Index::writePositions(QDataStream &out,
                           const QVector<quint32> &positions) {
  quint32 prev = 0;
  writeVInt(out, positions.size());
  foreach (quint32 position, positions) {
    // Convert to delta from absolute.
    quint32 delta = position - prev;
    writeVInt(out, delta);
    prev = position;
  }
}

bool Index::isLoggingEnabled() {
  return sLoggingEnabled || QSettings().value(kLogKey).toBool();
}

void Index::setLoggingEnabled(bool enable) {
  sLoggingEnabled = enable;
  QSettings().setValue(kLogKey, enable);
}

QDir Index::indexDir() const { return indexDir(mRepo); }
