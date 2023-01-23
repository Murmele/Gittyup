//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef LOGDELEGATE_H
#define LOGDELEGATE_H

#include <QDate>
#include <QStyledItemDelegate>
#include <QTimer>

class QTextDocument;

class LogDelegate : public QStyledItemDelegate {
public:
  LogDelegate(QObject *parent = 0);
  ~LogDelegate() override;

  virtual QSize sizeHint(const QStyleOptionViewItem &option,
                         const QModelIndex &index) const override;

  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                     const QModelIndex &index) const override;

  void invalidateCache(const QModelIndex &index);
  QTextDocument *document(const QModelIndex &index) const;
  QPoint documentPosition(const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

  QRect decorationRect(const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;

protected:
  void initStyleOption(QStyleOptionViewItem *option,
                       const QModelIndex &index) const override;

private:
  QStyle *style(const QStyleOptionViewItem &option) const;
  int verticalMargin(const QStyleOptionViewItem &option) const;
  int horizontalMargin(const QStyleOptionViewItem &option) const;

  QDate mCacheDate;
  QTimer mCacheTimer;
  mutable QMap<QModelIndex, QTextDocument *> mDocumentCache;
};

#endif
