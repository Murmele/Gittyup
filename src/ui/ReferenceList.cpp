//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ReferenceList.h"
#include "ReferenceView.h"
#include "git/Repository.h"
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>

namespace {

const QString kStyleSheet = "ReferenceList {"
                            "  combobox-popup: 0"
                            "}";

QModelIndex findReference(QAbstractItemModel *model,
                          const git::Reference &ref) {
  // Match by name.
  QModelIndexList indexes = model->match(
      model->index(0, 0), Qt::DisplayRole, ref.name(), 1,
      Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
  return !indexes.isEmpty() ? indexes.first() : QModelIndex();
}

} // namespace

ReferenceList::ReferenceList(const git::Repository &repo,
                             ReferenceView::Kinds kinds, QWidget *parent)
    : QComboBox(parent), mRepo(repo) {
  setStyleSheet(kStyleSheet);
  setMaxVisibleItems(0); // disable
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  mView = new ReferenceView(repo, kinds, true, this);
  QAbstractItemModel *model = mView->model();
  setModel(model);
  setView(mView);

  initIndex();

  connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged),
          [this](int index) {
            if (index >= 0)
              emit referenceSelected(currentReference());
          });

  connect(model, &QAbstractItemModel::modelAboutToBeReset,
          [this] { mStoredRef = currentReference(); });

  connect(model, &QAbstractItemModel::modelReset, [this] {
    // Try to restore the previous selection.
    if (count() > 0 && mStoredRef.isValid())
      select(mStoredRef, false);

    if (!currentReference().isValid())
      select(mRepo.head());
  });
}

git::Commit ReferenceList::target() const {
  git::Reference ref = currentReference();
  return ref.isValid() ? ref.target() : mCommit;
}

git::Reference ReferenceList::currentReference() const {
  return currentData(Qt::UserRole).value<git::Reference>();
}

void ReferenceList::initIndex() {
  // Select index. Priority: Branch --> Tag --> Remote --> Commit ID
  QModelIndex idx;
  if ((idx = mView->firstBranch()).isValid())
    setRootModelIndex(model()->index(0, 0));
  else if ((idx = mView->firstTag()).isValid())
    setRootModelIndex(model()->index(2, 0));
  else if ((idx = mView->firstRemote()).isValid())
    setRootModelIndex(model()->index(1, 0));
  else {
    setRootModelIndex(model()->index(0, 0));
    idx = model()->index(0, 0);
  }
  setCurrentIndex(idx.row());
}

void ReferenceList::setCommit(const git::Commit &commit) {
  mCommit = commit;
  mView->setCommit(commit);
  initIndex();
}

void ReferenceList::select(const git::Reference &ref, bool spontaneous) {
  if (!ref.isValid()) {
    emit referenceSelected(git::Reference());
    return;
  }

  QModelIndex index = findReference(model(), ref);
  if (!index.isValid())
    return;

  // Notify if the selection doesn't change.
  if (index.parent() == rootModelIndex() && index.row() == currentIndex()) {
    if (spontaneous)
      emit referenceSelected(ref);
  } else {
    setRootModelIndex(index.parent());
    setCurrentIndex(index.row());
  }
}

QSize ReferenceList::sizeHint() const {
  QComboBox::SizeAdjustPolicy policy = sizeAdjustPolicy();
  if (policy != QComboBox::AdjustToContents &&
      policy != QComboBox::AdjustToContentsOnFirstShow)
    return QComboBox::sizeHint();

  return calculateSizeHint();
}

QSize ReferenceList::minimumSizeHint() const {
  QComboBox::SizeAdjustPolicy policy = sizeAdjustPolicy();
  if (policy != QComboBox::AdjustToContents &&
      policy != QComboBox::AdjustToContentsOnFirstShow)
    return QComboBox::minimumSizeHint();

  return calculateSizeHint();
}

void ReferenceList::wheelEvent(QWheelEvent *event) { event->ignore(); }

void ReferenceList::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setPen(palette().color(QPalette::Text));

  QStyleOptionComboBox opt;
  initStyleOption(&opt);

  style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);

  if (opt.currentText.isEmpty()) {
    if (!mCommit.isValid())
      return;

    opt.currentText = mCommit.shortId();
  }

  QRect rect = style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                       QStyle::SC_ComboBoxEditField, this);
  painter.save();
  painter.setClipRect(rect);

  // Draw kind.
  git::Reference ref = currentReference();
  QString kind = ref.isValid() ? ReferenceView::kindString(ref) : tr("Commit");
  if (!kind.isEmpty()) {
    painter.save();
    painter.setPen(palette().color(QPalette::Disabled, QPalette::Text));
    QString text = QString("%1: ").arg(kind);
    style()->drawItemText(&painter, rect.adjusted(1, 0, -1, 0),
                          QStyle::visualAlignment(
                              opt.direction, Qt::AlignLeft | Qt::AlignVCenter),
                          opt.palette, opt.state & QStyle::State_Enabled, text);
    rect.setX(rect.x() + opt.fontMetrics.horizontalAdvance(text));
    painter.restore();
  }

  // Draw reference.
  if (ref.isValid() && ref.isHead()) {
    QFont bold = painter.font();
    bold.setBold(true);
    painter.setFont(bold);
  }

  style()->drawItemText(
      &painter, rect.adjusted(1, 0, -1, 0),
      QStyle::visualAlignment(opt.direction, Qt::AlignLeft | Qt::AlignVCenter),
      opt.palette, opt.state & QStyle::State_Enabled, opt.currentText);

  painter.restore();
}

QSize ReferenceList::calculateSizeHint() const {
  const QFontMetrics &fm = fontMetrics();
  int width = 24 * fm.horizontalAdvance(QLatin1Char('x'));
  int height = qMax(qCeil(QFontMetricsF(fm).height()), 14) + 2;

  // Adjust to contents.
  int rows = model()->rowCount();
  for (int row = 0; row < rows; ++row) {
    QModelIndex index = model()->index(row, 0);
    width = qMax(width, fm.boundingRect(index.data().toString()).width());
  }

  // Add style values.
  QStyleOptionComboBox opt;
  initStyleOption(&opt);
  QSize size(width, height);
  return style()->sizeFromContents(QStyle::CT_ComboBox, &opt, size, this);
}
