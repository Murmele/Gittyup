//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ReferenceWidget.h"
#include "ExpandButton.h"
#include "ConfigKeys.h"
#include "git/Config.h"
#include "git/Reference.h"
#include "git/Repository.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace {

const QString kBoldFmt = "<b>%1</b>";
const QString kNameFmt = "<span style='color: %1'>%2:</span> %3";
const QString kLinkFmt =
    "<a href='ref' style='color: %1; text-decoration: none'>%2</a>";

QModelIndex findReference(QAbstractItemModel *model,
                          const git::Reference &ref) {
  // Match by name.
  QModelIndexList indexes = model->match(
      model->index(0, 0), Qt::DisplayRole, ref.name(), 1,
      Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
  return !indexes.isEmpty() ? indexes.first() : QModelIndex();
}

class Label : public QLabel {
public:
  Label(QAbstractButton *button, QWidget *parent = nullptr)
      : QLabel(parent), mButton(button) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  }

protected:
  void mouseMoveEvent(QMouseEvent *event) override {
    QLabel::mouseMoveEvent(event);

    if (!(event->buttons() & Qt::LeftButton) ||
        cursor().shape() == Qt::PointingHandCursor)
      return;

    bool down = mButton->isDown();
    if (rect().contains(event->pos()) != down)
      mButton->setDown(!down);
  }

  void mousePressEvent(QMouseEvent *event) override {
    QLabel::mousePressEvent(event);

    if (event->button() != Qt::LeftButton ||
        cursor().shape() == Qt::PointingHandCursor)
      return;

    mButton->setDown(true);
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    QLabel::mouseReleaseEvent(event);

    if (event->button() != Qt::LeftButton ||
        cursor().shape() == Qt::PointingHandCursor)
      return;

    if (mButton->isDown())
      mButton->click();
    mButton->setDown(false);
  }

private:
  QAbstractButton *mButton;
};

class SelectionModel : public QItemSelectionModel {
public:
  SelectionModel(QAbstractItemModel *model, const git::Repository &repo)
      : QItemSelectionModel(model), mRepo(repo) {}

  void setCurrentIndex(const QModelIndex &index,
                       QItemSelectionModel::SelectionFlags command) override {
    QModelIndex current = index;
    git::Reference head = mRepo.head();
    bool all = mRepo.appConfig().value<bool>(ConfigKeys::kRefsKey, true);
    git::Reference ref = index.data(Qt::UserRole).value<git::Reference>();
    if (all && ref && !ref.isHead() && !ref.isStash() && head.isValid())
      current = findReference(model(), head);

    QItemSelectionModel::setCurrentIndex(current, command);
  }

private:
  git::Repository mRepo;
};

} // namespace

ReferenceWidget::ReferenceWidget(const git::Repository &repo,
                                 ReferenceView::Kinds kinds, QWidget *parent)
    : QFrame(parent), mRepo(repo) {
  setStyleSheet("ReferenceWidget {"
                "  border: 1px solid palette(shadow)"
                "}"
                "ReferenceWidget QToolButton {"
                "  border: none;"
                "  border-left: 1px solid palette(shadow)"
                "}"
                "ReferenceWidget QLabel {"
                "  padding-left: 4px;"
                "  background: palette(base)"
                "}"
                "ReferenceWidget QTreeView {"
                "  border: none"
                "}");

  ExpandButton *button = new ExpandButton(this);
  mLabel = new Label(button, this);
  connect(mLabel, &QLabel::linkActivated,
          [this] { emit referenceSelected(currentReference()); });

  QHBoxLayout *header = new QHBoxLayout;
  header->addWidget(mLabel);
  header->addWidget(button);

  mView = new ReferenceView(repo, kinds, false, this);
  mView->setSelectionModel(new SelectionModel(mView->model(), repo));

  // Close the drop-down list of branches if it has just been checked out.
  connect(mView, &ReferenceView::checkedOut, [button] { button->click(); });

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addLayout(header);
  layout->addWidget(mView);

  // Start hidden.
  mView->setVisible(false);
  connect(button, &ExpandButton::toggled, mView, &ReferenceView::setVisible);

  // Handle selection change.
  QItemSelectionModel *selection = mView->selectionModel();
  connect(selection, &QItemSelectionModel::currentChanged, [this] {
    if (!mSpontaneous)
      return;

    emit referenceChanged(currentReference());
    if (mView->isVisible())
      mView->setFocus();
  });

  connect(mView, &ReferenceView::clicked, [this](const QModelIndex &index) {
    emit referenceSelected(index.data(Qt::UserRole).value<git::Reference>());
  });

  // Handle model reset.
  QAbstractItemModel *model = mView->model();
  connect(model, &QAbstractItemModel::modelAboutToBeReset,
          [this] { mStoredRef = currentReference(); });

  connect(model, &QAbstractItemModel::modelReset, [this] {
    // Try to restore the previous selection.
    if (mStoredRef.isValid()) {
      mSpontaneous = false;
      select(mStoredRef);
      mSpontaneous = true;
    }

    if (!currentReference().isValid())
      select(mRepo.head());
  });
}

/*!
 * \brief ReferenceWidget::updateReference
 * Update Label
 * \param ref
 */
void ReferenceWidget::updateLabel(const git::Reference &ref) {
  if (!ref.isValid()) {
    mLabel->setText(QString());
    return;
  }

  QPalette palette = this->palette();
  QString enabled = palette.color(QPalette::Text).name();
  QString disabled = palette.color(QPalette::BrightText).name();

  QString kind = ReferenceView::kindString(ref);
  QString link = kLinkFmt.arg(enabled, ref.name());
  QString name = ref.isHead() ? kBoldFmt.arg(link) : link;
  mLabel->setText(kind.isEmpty() ? name : kNameFmt.arg(disabled, kind, name));
}

git::Reference ReferenceWidget::currentReference() const {
  git::Reference ref = mView->currentReference();
  bool all = mRepo.appConfig().value<bool>(ConfigKeys::kRefsKey, true);
  return (!all || (ref.isValid() && ref.isStash())) ? ref : mRepo.head();
}

/*!
 * \brief ReferenceWidget::select
 * \param ref
 * \param suppress
 */
void ReferenceWidget::select(const git::Reference &ref, bool suppress) {
  if (!ref.isValid()) {
    updateLabel(ref);
    if (!suppress)
      emit referenceChanged(git::Reference());
    return;
  }

  QModelIndex index = findReference(mView->model(), ref);
  if (!index.isValid())
    return;

  if (index == mView->currentIndex()) {
    updateLabel(ref);
    if (!suppress)
      emit referenceChanged(ref);
    return;
  }

  mView->setRootIndex(index.parent());
  mView->setCurrentIndex(index);
  mView->resetTabIndex();
}
