//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ReferenceView.h"
#include "RepoView.h"
#include "TabBar.h"
#include "dialogs/DeleteBranchDialog.h"
#include "dialogs/DeleteTagDialog.h"
#include "dialogs/MergeDialog.h"
#include "git/Branch.h"
#include "git/Repository.h"
#include "git/TagRef.h"
#include "log/LogEntry.h"
#include <QAbstractItemModel>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

#include "ReferenceModel.h"

namespace {

const int kHeight = 200;

class FilterProxyModel : public QSortFilterProxyModel {
public:
  FilterProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

  void setFilter(const QString &filter) {
    setFilterRegExp(QRegExp(filter, Qt::CaseInsensitive, QRegExp::FixedString));
  }

protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override {
    if (!parent.isValid())
      return true;

    return QSortFilterProxyModel::filterAcceptsRow(row, parent);
  }
};

class Delegate : public QStyledItemDelegate {
public:
  Delegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

protected:
  void initStyleOption(QStyleOptionViewItem *option,
                       const QModelIndex &index) const {
    QStyledItemDelegate::initStyleOption(option, index);
    option->state |= QStyle::State_Active;
  }
};

class Header : public QHeaderView {
  Q_OBJECT

public:
  Header(ReferenceView *view = nullptr) : QHeaderView(Qt::Horizontal, view) {
    setStretchLastSection(true);

    mTabs = new TabBar(this);
    connect(mTabs, &TabBar::currentChanged, [this, view](int tab) {
      QAbstractItemModel *model = view->model();
      QModelIndex index = model->index(tab, 0);
      view->setRootIndex(index);

      QString name = index.data().toString();
      mField->setPlaceholderText(tr("Filter %1").arg(name));

      QModelIndex child = model->index(0, 0, index);
      view->setRootIsDecorated(model->rowCount(child) > 0);

      // Select the first index.
      QModelIndex parent = view->currentIndex().parent();
      if (parent != view->rootIndex() && view->isPopup())
        view->setCurrentIndex(child);
    });

    // Forward up/down key presses from field to view.
    mField = new QLineEdit(this);
    mField->installEventFilter(view);
    mField->setClearButtonEnabled(true);

    QHBoxLayout *fieldLayout = new QHBoxLayout;
    fieldLayout->setContentsMargins(2, 2, 2, 2);
    fieldLayout->addWidget(mField);

    mWidget = new QWidget(this);
    mWidget->setAutoFillBackground(true);

    QVBoxLayout *layout = new QVBoxLayout(mWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(mTabs);
    layout->addLayout(fieldLayout);
  }

  QLineEdit *field() const { return mField; }
  QTabBar *tabs() const { return mTabs; }

  QSize sizeHint() const override { return mWidget->sizeHint(); }

protected:
  void hideEvent(QHideEvent *event) override { mField->clear(); }

  void resizeEvent(QResizeEvent *event) override {
    QHeaderView::resizeEvent(event);
    mWidget->resize(size());
  }

private:
  QLineEdit *mField;
  QTabBar *mTabs;
  QWidget *mWidget;
};

} // namespace

ReferenceView::ReferenceView(const git::Repository &repo, Kinds kinds,
                             bool popup, QWidget *parent)
    : QTreeView(parent), mPopup(popup) {
  // Constrain height.
  setMinimumHeight(kHeight);
  setMaximumHeight(kHeight);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  if (popup)
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // Set delegate.
  setItemDelegate(new Delegate(this));

  // Set header.
  Header *header = new Header(this);
  setHeader(header);

  QTabBar *tabs = header->tabs();
  QLineEdit *field = header->field();

  // Set model.
  FilterProxyModel *model = new FilterProxyModel(this);
  mSource = new ReferenceModel(repo, kinds, this);
  model->setSourceModel(mSource);
  setModel(model);

  connect(field, &QLineEdit::textChanged, [this, model](const QString &text) {
    model->setFilter(text);
    QModelIndex root = rootIndex();
    if (!currentIndex().isValid() && model->rowCount(root) > 0)
      setCurrentIndex(model->index(0, 0, root));
  });

  connect(model, &QAbstractItemModel::modelReset, [this, tabs] {
    // Reset tabs.
    while (tabs->count() > 0)
      tabs->removeTab(0);

    QAbstractItemModel *model = this->model();
    for (int i = 0; i < model->rowCount(); ++i)
      tabs->addTab(model->index(i, 0).data().toString());
  });

  if (!popup) {
    // Checkout on double-click.
    connect(this, &ReferenceView::doubleClicked,
            [this, repo](const QModelIndex &index) {
              if (repo.isBare())
                return;

              git::Reference ref =
                  index.data(Qt::UserRole).value<git::Reference>();
              if (ref.isValid() && !ref.isHead())
                checkout(ref);
            });
  }

  // Keep focus on the field.
  setFocusProxy(field);

  // Update model last.
  static_cast<ReferenceModel *>(mSource)->update();
}

void ReferenceView::setCommit(const git::Commit &commit) {
  static_cast<ReferenceModel *>(mSource)->setCommit(commit);
}

void ReferenceView::resetTabIndex() {
  QModelIndex root = currentIndex();
  while (root.parent().isValid())
    root = root.parent();

  static_cast<Header *>(header())->tabs()->setCurrentIndex(root.row());
}

QModelIndex ReferenceView::firstBranch() {
  auto index = static_cast<ReferenceModel *>(mSource)->firstBranch();
  index = static_cast<FilterProxyModel *>(model())->mapFromSource(index);
  if (!index.isValid())
    return QModelIndex();
  return index;
}

QModelIndex ReferenceView::firstTag() {
  auto index = static_cast<ReferenceModel *>(mSource)->firstTag();
  index = static_cast<FilterProxyModel *>(model())->mapFromSource(index);
  if (!index.isValid())
    return QModelIndex();
  return index;
}

QModelIndex ReferenceView::firstRemote() {
  auto index = static_cast<ReferenceModel *>(mSource)->firstRemote();
  index = static_cast<FilterProxyModel *>(model())->mapFromSource(index);
  if (!index.isValid())
    return QModelIndex();
  return index;
}

git::Reference ReferenceView::currentReference() const {
  return currentIndex().data(Qt::UserRole).value<git::Reference>();
}

bool ReferenceView::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() != QEvent::KeyPress)
    return false;

  QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
  switch (keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
      keyPressEvent(keyEvent);
      return true;
  }

  return false;
}

QString ReferenceView::kindString(const git::Reference &ref) {
  if (ref.isLocalBranch())
    return tr("Branch");

  if (ref.isRemoteBranch())
    return tr("Remote");

  if (ref.isTag())
    return tr("Tag");

  return QString();
}

void ReferenceView::checkout(git::Reference &ref) {
  RepoView::parentView(this)->checkout(ref);
  emit checkedOut();
}

void ReferenceView::showEvent(QShowEvent *event) {
  resetTabIndex();
  setFocus();

  QTreeView::showEvent(event);
}

void ReferenceView::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex index = indexAt(event->pos());
  git::Reference ref = index.data(Qt::UserRole).value<git::Reference>();
  if (!ref.isValid())
    return;

  QMenu menu;
  QAction *checkout =
      menu.addAction(tr("Checkout"), [this, &ref] { this->checkout(ref); });

  RepoView *view = RepoView::parentView(this);
  checkout->setEnabled(!ref.isHead() && !view->repo().isBare());

  menu.addSeparator();

  if (ref.isLocalBranch()) {
    QAction *rename = menu.addAction(tr("Rename"), [this, ref] {
      RepoView *view = RepoView::parentView(this);
      ConfigDialog *dialog = view->configureSettings(ConfigDialog::Branches);
      dialog->editBranch(ref.name());
    });

    rename->setEnabled(!ref.isHead());
  }

  if (ref.isTag() || ref.isLocalBranch()) {
    QAction *remove = menu.addAction(tr("Delete"), [this, ref] {
      if (ref.isTag()) {
        DeleteTagDialog *dialog = new DeleteTagDialog(ref, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();

      } else {
        DeleteBranchDialog *dialog = new DeleteBranchDialog(ref, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
      }
    });

    remove->setEnabled(ref.isTag() || !ref.isHead());
  }

  if (ref.isTag()) {
    git::Remote remote = ref.repo().defaultRemote();
    if (remote.isValid()) {
      menu.addAction(tr("Push Tag to %1").arg(remote.name()),
                     [ref, view, remote] { view->push(remote, ref); });
    }
  }

  if (ref.isRemoteBranch()) {
    menu.addAction(tr("New Local Branch"), [this, ref] {
      QString local = ref.name().section('/', 1);
      RepoView::parentView(this)->createBranch(local, ref.target(), ref, true);
    });
  }

  menu.addSeparator();

  QAction *merge = menu.addAction(tr("Merge..."), [this, ref] {
    RepoView *view = RepoView::parentView(this);
    MergeDialog *dialog = new MergeDialog(RepoView::Merge, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->setReference(ref);
    dialog->open();
  });

  QAction *rebase = menu.addAction(tr("Rebase..."), [this, ref] {
    RepoView *view = RepoView::parentView(this);
    MergeDialog *dialog = new MergeDialog(RepoView::Rebase, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->setReference(ref);
    dialog->open();
  });

  QAction *squash = menu.addAction(tr("Squash..."), [this, ref] {
    RepoView *view = RepoView::parentView(this);
    MergeDialog *dialog = new MergeDialog(RepoView::Squash, view->repo(), view);
    connect(dialog, &QDialog::accepted, [view, dialog] {
      view->merge(dialog->flags(), dialog->reference());
    });

    dialog->setReference(ref);
    dialog->open();
  });

  bool stash = ref.isStash();
  merge->setEnabled(!stash);
  rebase->setEnabled(!stash);
  squash->setEnabled(!stash);

  menu.exec(event->globalPos());
}

#include "ReferenceView.moc"
