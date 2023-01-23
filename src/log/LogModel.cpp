//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "LogModel.h"
#include "LogEntry.h"
#include <QStyle>

namespace {

const QString kTitleFmt = "<b>%1</b> - %2";
const QString kTimeFmt = "<span style='color: #808080'>%1</span> - %2";

} // namespace

LogModel::LogModel(LogEntry *root, QStyle *style, QObject *parent)
    : QAbstractItemModel(parent), mRoot(root),
      mHintIcon(style->standardIcon(QStyle::SP_MessageBoxInformation)),
      mWarningIcon(style->standardIcon(QStyle::SP_MessageBoxWarning)),
      mErrorIcon(style->standardIcon(QStyle::SP_MessageBoxCritical)) {
  mRoot->setParent(this);
  connect(root, &LogEntry::entriesAboutToBeInserted,
          [this](LogEntry *entry, int row, int count) {
            beginInsertRows(index(entry), row, row + count - 1);
          });

  connect(root, &LogEntry::entriesInserted, this, &LogModel::endInsertRows);

  connect(root, &LogEntry::dataChanged, [this](LogEntry *entry) {
    QModelIndex index = this->index(entry);
    emit dataChanged(index, index, {Qt::DisplayRole});
  });
}

QModelIndex LogModel::index(int row, int column,
                            const QModelIndex &parent) const {
  QList<LogEntry *> entries = entry(parent)->entries();
  bool invalid = (row < 0 || row >= entries.size());
  return invalid ? QModelIndex() : createIndex(row, column, entries.at(row));
}

QModelIndex LogModel::parent(const QModelIndex &index) const {
  LogEntry *entry = this->entry(index)->parentEntry();
  if (entry == mRoot)
    return QModelIndex();

  LogEntry *parent = entry->parentEntry();
  return createIndex(parent->entries().indexOf(entry), 0, entry);
}

int LogModel::rowCount(const QModelIndex &parent) const {
  return entry(parent)->entries().size();
}

int LogModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant LogModel::data(const QModelIndex &index, int role) const {
  LogEntry *entry = this->entry(index);
  switch (role) {
    case Qt::DisplayRole:
      switch (entry->kind()) {
        case LogEntry::Entry: {
          QString text = entry->text();
          QString title = entry->title();
          if (!title.isEmpty()) {
            text = kTitleFmt.arg(title, text);

            if (!index.parent().isValid()) {
              QDateTime date = entry->timestamp();
              QString timestamp =
                  (date.date() == QDate::currentDate())
                      ? date.time().toString(Qt::DefaultLocaleShortDate)
                      : date.toString(Qt::DefaultLocaleShortDate);
              text = kTimeFmt.arg(timestamp, text);
            }
          }

          return text;
        }

        case LogEntry::File:
        case LogEntry::Hint:
        case LogEntry::Warning:
        case LogEntry::Error:
          return entry->text();
      }

    case Qt::DecorationRole:
      switch (entry->kind()) {
        case LogEntry::Entry: {
          int progress = entry->progress();
          return (progress < 0) ? QVariant() : progress;
        }

        case LogEntry::File:
          return QChar(entry->status());

        case LogEntry::Hint:
          return mHintIcon;

        case LogEntry::Warning:
          return mWarningIcon;

        case LogEntry::Error:
          return mErrorIcon;
      }

    case EntryRole:
      return QVariant::fromValue<LogEntry *>(entry);
  }

  return QVariant();
}

QModelIndex LogModel::index(LogEntry *entry) const {
  LogEntry *parent = entry->parentEntry();
  return parent ? createIndex(parent->entries().indexOf(entry), 0, entry)
                : QModelIndex();
}

LogEntry *LogModel::entry(const QModelIndex &index) const {
  void *ptr = index.internalPointer();
  return index.isValid() ? static_cast<LogEntry *>(ptr) : mRoot;
}
