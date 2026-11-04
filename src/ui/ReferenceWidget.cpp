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
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
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

  // Set the reference to display. The branch name is elided with an ellipsis
  // to fit the available width so the widget (and the sidebar) can shrink.
  void setReference(const QString &kind, const QString &name,
                    const QString &enabled, const QString &disabled,
                    bool head) {
    mKind = kind;
    mName = name;
    mEnabled = enabled;
    mDisabled = disabled;
    mHead = head;
    updateText();
  }

  QSize minimumSizeHint() const override {
    // Don't let the full branch name text dictate the minimum width, otherwise
    // the sidebar can't be made narrower than the longest branch name.
    QSize hint = QLabel::minimumSizeHint();
    hint.setWidth(QFontMetrics(font()).horizontalAdvance(QStringLiteral("…")));
    return hint;
  }

protected:
  void resizeEvent(QResizeEvent *event) override {
    QLabel::resizeEvent(event);
    updateText();
  }

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
  // Rebuild the label text, eliding the branch name to fit the current width.
  void updateText() {
    if (mName.isEmpty()) {
      QLabel::clear();
      return;
    }

    int available = contentsRect().width();

    // Reserve room for the "kind:" prefix, which is never elided.
    QFontMetrics fm(font());
    if (!mKind.isEmpty())
      available -= fm.horizontalAdvance(mKind + QStringLiteral(": "));

    // The head reference is shown in bold, so measure with a bold font.
    QFont nameFont = font();
    nameFont.setBold(mHead);
    QString elided = QFontMetrics(nameFont)
                         .elidedText(mName, Qt::ElideRight, qMax(available, 0));

    QString link = kLinkFmt.arg(mEnabled, elided);
    QString name = mHead ? kBoldFmt.arg(link) : link;
    QLabel::setText(mKind.isEmpty() ? name
                                    : kNameFmt.arg(mDisabled, mKind, name));
  }

  QAbstractButton *mButton;

  QString mKind;
  QString mName;
  QString mEnabled;
  QString mDisabled;
  bool mHead = false;
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
  Label *label = static_cast<Label *>(mLabel);
  if (!ref.isValid()) {
    label->setReference(QString(), QString(), QString(), QString(), false);
    return;
  }

  QPalette palette = this->palette();
  QString enabled = palette.color(QPalette::Text).name();
  QString disabled = palette.color(QPalette::BrightText).name();

  QString kind = ReferenceView::kindString(ref);
  label->setReference(kind, ref.name(), enabled, disabled, ref.isHead());
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
