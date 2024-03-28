//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "LogDelegate.h"
#include "ui/Badge.h"
#include "ui/ProgressIndicator.h"
#include <QApplication>
#include <QPainter>
#include <QTextDocument>

#ifdef Q_OS_WIN
#define VERTICAL_MARGIN 2
#define ADJUSTMENT_SIZE QSize(2, 2)
#else
#define VERTICAL_MARGIN 1
#define ADJUSTMENT_SIZE QSize(0, 0)
#endif

LogDelegate::LogDelegate(QObject *parent) : QStyledItemDelegate(parent) {
  mCacheDate = QDate::currentDate();
  mCacheTimer.setTimerType(Qt::VeryCoarseTimer);
  connect(&mCacheTimer, &QTimer::timeout, [this] {
    QDate date = QDate::currentDate();
    if (mCacheDate != date) {
      qDeleteAll(mDocumentCache);
      mDocumentCache.clear();
      mCacheDate = date;
    }
  });

  mCacheTimer.start(60 * 1000);
}

LogDelegate::~LogDelegate() { qDeleteAll(mDocumentCache); }

QSize LogDelegate::sizeHint(const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);

  int decWidth = 0;
  int decHeight = 0;
  if (opt.features & QStyleOptionViewItem::HasDecoration) {
    decWidth = opt.decorationSize.width() + (horizontalMargin(opt) * 2);
    decHeight = opt.decorationSize.height() + (verticalMargin(opt) * 2);
  }

  QTextDocument *doc = document(index);
  int docWidth = doc->idealWidth() + (horizontalMargin(opt) * 2);
  int docHeight = doc->size().height() + (verticalMargin(opt) * 2);
  return QSize(decWidth + docWidth, qMax(docHeight, decHeight));
}

void LogDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const {
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);

  // Draw background.
  opt.text = QString();
  style(opt)->drawControl(QStyle::CE_ItemViewItem, &opt, painter);

  // Draw text.
  painter->save();
  painter->translate(documentPosition(option, index));
  document(index)->drawContents(painter);
  painter->restore();

  // Draw decoration.
  QVariant variant = index.data(Qt::DecorationRole);
  if (!variant.isValid())
    return;

  QRect rect = decorationRect(option, index);
  if (variant.typeId() == QMetaType::QChar) {
    Badge::paint(painter,
                 {Badge::Label(Badge::Label::Type::Log, variant.toChar())},
                 rect, &opt);
  } else if (variant.canConvert<int>()) {
    int progress = variant.toInt();
    ProgressIndicator::paint(painter, rect, "#808080", progress, opt.widget);
  }
}

void LogDelegate::invalidateCache(const QModelIndex &index) {
  delete mDocumentCache.take(index);
}

QTextDocument *LogDelegate::document(const QModelIndex &index) const {
  auto it = mDocumentCache.find(index);
  if (it != mDocumentCache.end())
    return it.value();

  QTextDocument *doc = new QTextDocument(const_cast<LogDelegate *>(this));
  doc->setHtml(index.data().toString());
  doc->setDocumentMargin(0);
  mDocumentCache.insert(index, doc);
  return doc;
}

QPoint LogDelegate::documentPosition(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);

  QRect rect = style(opt)->subElementRect(QStyle::SE_ItemViewItemText, &opt);
  int y = (rect.height() / 2) - (document(index)->size().height() / 2);
  return rect.topLeft() + QPoint(horizontalMargin(opt), y);
}

QRect LogDelegate::decorationRect(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const {
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);

  QStyle::SubElement se = QStyle::SE_ItemViewItemDecoration;
  return style(opt)->subElementRect(se, &opt, opt.widget);
}

void LogDelegate::initStyleOption(QStyleOptionViewItem *option,
                                  const QModelIndex &index) const {
  QStyledItemDelegate::initStyleOption(option, index);
  QVariant variant = index.data(Qt::DecorationRole);
  if (variant.typeId() == QMetaType::QChar) {
    option->decorationSize =
        Badge::size(option->font, Badge::Label(Badge::Label::Type::Log)) -
        ADJUSTMENT_SIZE;
  } else if (variant.canConvert<int>()) {
    option->decorationSize = ProgressIndicator::size();
  }
}

QStyle *LogDelegate::style(const QStyleOptionViewItem &opt) const {
  return opt.widget ? opt.widget->style() : QApplication::style();
}

int LogDelegate::verticalMargin(const QStyleOptionViewItem &opt) const {
  return VERTICAL_MARGIN;
}

int LogDelegate::horizontalMargin(const QStyleOptionViewItem &opt) const {
  return style(opt)->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, opt.widget) +
         1;
}
