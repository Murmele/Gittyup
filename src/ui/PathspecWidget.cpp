//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "PathspecWidget.h"
#include "ExpandButton.h"
#include "FileContextMenu.h"
#include "RepoView.h"
#include "TreeModel.h"
#include "git/Reference.h"
#include "git/Repository.h"
#include <QAbstractProxyModel>
#include <QCompleter>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

const int kHeight = 200;

class TreeView : public QTreeView {
public:
  TreeView(const git::Repository &repo, QWidget *parent = nullptr)
      : QTreeView(parent) {
    setHeaderHidden(true);

    // Constrain height.
    setMinimumHeight(kHeight);
    setMaximumHeight(kHeight);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    TreeModel *model = new TreeModel(repo, this);
    setModel(model);

    if (git::Reference head = repo.head())
      model->setTree(head.target().tree());

    // Reset tree when HEAD changes.
    connect(repo.notifier(), &git::RepositoryNotifier::referenceUpdated, this,
            [model](const git::Reference &ref) {
              if (ref.isValid() && ref.isHead())
                model->setTree(ref.target().tree());
            });
  }

protected:
  void contextMenuEvent(QContextMenuEvent *event) override {
    QStringList files;
    foreach (const QModelIndex &index, selectionModel()->selectedIndexes())
      files.append(index.data(Qt::EditRole).toString());

    FileContextMenu menu(RepoView::parentView(this), files);
    menu.exec(event->globalPos());
  }
};

class Completer : public QCompleter {
public:
  Completer(QAbstractItemModel *model, QObject *parent = nullptr)
      : QCompleter(model, parent) {
    setCompletionRole(Qt::DisplayRole);
    setCompletionMode(QCompleter::InlineCompletion);
    setModelSorting(QCompleter::CaseSensitivelySortedModel);
  }

  QString pathFromIndex(const QModelIndex &index) const override {
    return index.data(Qt::EditRole).toString();
  }

  QStringList splitPath(const QString &path) const override {
    return path.split('/');
  }
};

} // namespace

PathspecWidget::PathspecWidget(const git::Repository &repo, QWidget *parent)
    : QFrame(parent) {
  setStyleSheet("PathspecWidget {"
                "  border: 1px solid palette(shadow)"
                "}"
                "PathspecWidget QLineEdit {"
                "  border: 1px solid palette(base);"
                "  padding-left: 4px"
                "}"
                "PathspecWidget QToolButton {"
                "  border: none;"
                "  border-left: 1px solid palette(shadow)"
                "}"
                "PathspecWidget QTreeView {"
                "  border: none;"
                "  border-top: 1px solid palette(mid)"
                "}");

  mField = new QLineEdit(this);
  mField->setClearButtonEnabled(true);
  mField->setPlaceholderText(tr("Filter by Path"));

  ExpandButton *button = new ExpandButton(this);

  QHBoxLayout *header = new QHBoxLayout;
  header->addWidget(mField);
  header->addWidget(button);

  mView = new TreeView(repo, this);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addLayout(header);
  layout->addWidget(mView);

  // Start hidden.
  mView->setVisible(false);
  connect(button, &ExpandButton::toggled, mView, &QTreeView::setVisible);

  // Handle selection change.
  QItemSelectionModel *selection = mView->selectionModel();
  connect(selection, &QItemSelectionModel::currentChanged, [this] {
    QModelIndex index = mView->currentIndex();
    if (!mHighlighted)
      mField->setText(index.data(Qt::EditRole).toString());

    mField->removeAction(mIcon);
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    mIcon = icon.isNull() ? nullptr
                          : mField->addAction(icon, QLineEdit::LeadingPosition);
  });

  // Handle text change.
  connect(
      mField, &QLineEdit::textChanged, this,
      [this] {
        // Use the current text. It may be different from
        // the argument because this connection is queued.
        QString text = mField->text();
        emit pathspecChanged(text);
        if (text.isEmpty())
          mView->setCurrentIndex(QModelIndex());
      },
      Qt::QueuedConnection);

  // Set the completer after the view creates the model.
  mField->setCompleter(new Completer(mView->model(), mView));

  using Signal = void (QCompleter::*)(const QModelIndex &);
  auto signal = static_cast<Signal>(&QCompleter::highlighted);
  connect(mField->completer(), signal, [this](const QModelIndex &index) {
    QAbstractItemModel *model = mField->completer()->completionModel();
    QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(model);
    Q_ASSERT(proxy);

    mHighlighted = true;
    QModelIndex sourceIndex = proxy->mapToSource(index);
    mView->setCurrentIndex(sourceIndex);
    mView->scrollTo(sourceIndex);
    mHighlighted = false;
  });
}

QString PathspecWidget::pathspec() const { return mField->text(); }

void PathspecWidget::setPathspec(const QString &pathspec) {
  mField->setText(pathspec);
}
