//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>

class LogEntry : public QObject {
  Q_OBJECT

public:
  enum Kind { Entry, File, Hint, Warning, Error };

  LogEntry(QObject *parent = nullptr);
  LogEntry(Kind kind, const QString &text, const QString &title,
           LogEntry *parent = nullptr);

  Kind kind() const { return mKind; }

  char status() const { return mStatus; }
  void setStatus(char status) { mStatus = status; }

  QString text() const { return mText; }
  void setText(const QString &text);

  QString title() const { return mTitle; }
  QDateTime timestamp() const { return mTimestamp; }

  LogEntry *rootEntry() const;
  LogEntry *parentEntry() const;

  const QList<LogEntry *> &entries() const { return mEntries; }

  void addEntries(const QList<LogEntry *> &entries);
  LogEntry *addEntry(Kind kind, const QString &text);
  LogEntry *addEntry(const QString &text, const QString &title = QString());

  void insertEntries(int row, const QList<LogEntry *> &entries);
  LogEntry *insertEntry(int row, Kind kind, const QString &text,
                        const QString &title = QString());
  LogEntry *lastEntry() const;

  int progress() const { return mProgress; }
  void setBusy(bool busy);

signals:
  void dataChanged(LogEntry *entry);
  void entriesAboutToBeInserted(LogEntry *entry, int row, int count = 1);
  void entriesInserted();
  void errorInserted();

private:
  Kind mKind = Entry;
  char mStatus = 0;
  QString mText;
  QString mTitle;
  QDateTime mTimestamp;
  QList<LogEntry *> mEntries;

  QTimer mTimer;
  int mProgress = -1;
};

#endif
