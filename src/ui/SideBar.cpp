//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SideBar.h"
#include "Footer.h"
#include "MainWindow.h"
#include "ProgressIndicator.h"
#include "RepoView.h"
#include "TabWidget.h"
#include "app/Application.h"
#include "conf/RecentRepositories.h"
#include "conf/RecentRepository.h"
#include "dialogs/AccountDialog.h"
#include "dialogs/CloneDialog.h"
#include "host/Accounts.h"
#include <QAbstractItemModel>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

const QString kRemoteExpandedGroup = "remote/expanded";

const QString kStyleSheet = "QTreeView {"
#ifdef Q_OS_MAC
                            "  background: palette(midlight);"
#endif
                            "  border: none"
                            "}"
                            "Footer {"
                            "  border-left: none;"
                            "  border-right: none"
                            "}";

enum Role {
  PathRole = Qt::UserRole,
  TabRole,
  RecentRole,
  AccountRole,
  AccountKindRole,
  RepositoryRole
};

class ProgressDelegate : public QStyledItemDelegate {
public:
  ProgressDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Draw background.
    QStyledItemDelegate::paint(painter, opt, index);

    // Draw busy indicator.
    QVariant progress = index.data(Qt::DecorationRole);
    if (!progress.canConvert<int>())
      return;

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    QStyle::SubElement se = QStyle::SE_ItemViewItemDecoration;
    QRect rect = style->subElementRect(se, &opt, opt.widget);
    ProgressIndicator::paint(painter, rect, "#808080", progress.toInt());
  }

protected:
  void initStyleOption(QStyleOptionViewItem *option,
                       const QModelIndex &index) const override {
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.data(Qt::DecorationRole).canConvert<int>()) {
      option->decorationSize = ProgressIndicator::size();
    } else if (index.data(RepositoryRole).isValid()) {
      option->features |= QStyleOptionViewItem::HasDecoration;
      option->decorationSize = QSize(20, 20);
    }
  }
};

class RepoModel : public QAbstractItemModel {
  Q_OBJECT

public:
  enum RootRow { Repo, Recent, Remote };

  RepoModel(TabWidget *tabs, QObject *parent = nullptr)
      : QAbstractItemModel(parent), mTabs(tabs), mCloudIcon(":/cloud.png"),
        mErrorIcon(tabs->style()->standardIcon(QStyle::SP_MessageBoxCritical)) {
    connect(tabs, &TabWidget::tabAboutToBeInserted, this,
            &RepoModel::beginResetModel);
    connect(tabs, &TabWidget::tabAboutToBeRemoved, this,
            &RepoModel::beginResetModel);
    connect(tabs, QOverload<>::of(&TabWidget::tabInserted), this,
            &RepoModel::endResetModel);
    connect(tabs, QOverload<>::of(&TabWidget::tabRemoved), this,
            &RepoModel::endResetModel);
    connect(tabs->tabBar(), &QTabBar::tabMoved, [this] {
      beginResetModel();
      endResetModel();
    });

    RecentRepositories *repos = RecentRepositories::instance();
    connect(repos, &RecentRepositories::repositoryAboutToBeAdded, this,
            &RepoModel::beginResetModel);
    connect(repos, &RecentRepositories::repositoryAboutToBeRemoved, this,
            &RepoModel::beginResetModel);
    connect(repos, &RecentRepositories::repositoryAdded, this,
            &RepoModel::endResetModel);
    connect(repos, &RecentRepositories::repositoryRemoved, this,
            &RepoModel::endResetModel);

    Accounts *accounts = Accounts::instance();
    connect(accounts, &Accounts::accountAboutToBeAdded, this,
            &RepoModel::beginResetModel);
    connect(accounts, &Accounts::accountAdded, this, &RepoModel::endResetModel);
    connect(accounts, &Accounts::accountAboutToBeRemoved, this,
            &RepoModel::beginResetModel);
    connect(accounts, &Accounts::accountRemoved, this,
            &RepoModel::endResetModel);

    connect(accounts, &Accounts::progress, this, [this](int accountIndex) {
      QModelIndex idx = index(0, 0, index(accountIndex, 0, index(Remote, 0)));
      emit dataChanged(idx, idx, {Qt::DisplayRole});
    });
    connect(accounts, &Accounts::started, this, [this](int accountIndex) {
      beginResetModel();
      endResetModel();
    });
    connect(accounts, &Accounts::finished, this, [this](int accountIndex) {
      beginResetModel();
      endResetModel();
    });

    connect(accounts, &Accounts::repositoryAboutToBeAdded, this,
            &RepoModel::beginResetModel);
    connect(accounts, &Accounts::repositoryAdded, this,
            &RepoModel::endResetModel);
    connect(accounts, &Accounts::repositoryPathChanged, this,
            [this](int accountIndex, int repoIndex) {
              QModelIndex idx =
                  index(repoIndex, 0, index(accountIndex, 0, index(Remote, 0)));
              emit dataChanged(idx, idx, {Qt::DisplayRole});
            });
  }

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override {
    if (!parent.isValid())
      return createIndex(row, column);

    QObject *ptr = static_cast<QObject *>(parent.internalPointer());
    if (Account *account = qobject_cast<Account *>(ptr)) {
      // repository
      if (account->repositoryCount())
        return createIndex(row, column, account->repository(row));

      // error
      AccountError *error = account->error();
      if (error->isValid())
        return createIndex(row, column, error);

      // progress
      return createIndex(row, column, account->progress());
    }

    Accounts *accounts = Accounts::instance();
    RecentRepositories *recent = RecentRepositories::instance();
    switch (parent.row()) {
      case Repo:
        if (!mTabs->count())
          return createIndex(row, column, mTabs);
        return createIndex(row, column, mTabs->widget(row));

      case Recent:
        if (!recent->count())
          return createIndex(row, column, recent);
        return createIndex(row, column, recent->repository(row));

      case Remote:
        if (!accounts->count())
          return createIndex(row, column, accounts);
        return createIndex(row, column, accounts->account(row));
    }

    return QModelIndex();
  }

  QModelIndex parent(const QModelIndex &index) const override {
    QObject *ptr = static_cast<QObject *>(index.internalPointer());
    if (!ptr)
      return QModelIndex();

    if (qobject_cast<RepoView *>(ptr) || qobject_cast<TabWidget *>(ptr))
      return createIndex(Repo, 0);

    if (qobject_cast<RecentRepository *>(ptr) ||
        qobject_cast<RecentRepositories *>(ptr))
      return createIndex(Recent, 0);

    if (qobject_cast<Account *>(ptr) || qobject_cast<Accounts *>(ptr))
      return createIndex(Remote, 0);

    Account *account = nullptr;
    if (Repository *repo = qobject_cast<Repository *>(ptr)) {
      account = repo->account();
    } else if (AccountError *error = qobject_cast<AccountError *>(ptr)) {
      account = error->account();
    } else if (AccountProgress *ap = qobject_cast<AccountProgress *>(ptr)) {
      account = ap->account();
    }

    if (account)
      return createIndex(Accounts::instance()->indexOf(account), 0, account);

    return QModelIndex();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    if (!parent.isValid())
      return 3;

    QObject *ptr = static_cast<QObject *>(parent.internalPointer());
    if (Account *account = qobject_cast<Account *>(ptr)) {
      int count = account->repositoryCount();
      if (count > 0)
        return count;

      AccountError *error = account->error();
      AccountProgress *progress = account->progress();
      return (error->isValid() || progress->isValid()) ? 1 : 0;
    }

    if (parent.parent().isValid())
      return 0;

    switch (parent.row()) {
      case Repo:
        return mTabs->count() ? mTabs->count() : 1;

      case Recent: {
        RecentRepositories *recent = RecentRepositories::instance();
        return recent->count() ? recent->count() : 1;
      }

      case Remote: {
        Accounts *accounts = Accounts::instance();
        return accounts->count() ? accounts->count() : Account::NUM_KINDS;
      }

      default:
        return 0;
    }
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const override {
    return 1;
  }

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    QObject *ptr = static_cast<QObject *>(index.internalPointer());
    if (Repository *repo = qobject_cast<Repository *>(ptr)) {
      switch (role) {
        case Qt::DisplayRole:
          return mShowFullName ? repo->fullName() : repo->name();

        case Qt::DecorationRole: {
          Account *account = repo->account();
          QString path = account->repositoryPath(account->indexOf(repo));
          return path.isEmpty() ? mCloudIcon : QIcon();
        }

        case PathRole: {
          Account *account = repo->account();
          return account->repositoryPath(account->indexOf(repo));
        }

        case RepositoryRole:
          return QVariant::fromValue(repo);

        default:
          return QVariant();
      }
    }

    if (AccountError *error = qobject_cast<AccountError *>(ptr)) {
      switch (role) {
        case Qt::DisplayRole:
          return error->text();
        case Qt::DecorationRole:
          return mErrorIcon;
        case Qt::ToolTipRole:
          return error->detailedText();
        default:
          return QVariant();
      }
    }

    if (AccountProgress *progress = qobject_cast<AccountProgress *>(ptr)) {
      switch (role) {
        case Qt::DisplayRole:
          return tr("Connecting");

        case Qt::DecorationRole:
          return progress->value();

        case Qt::FontRole: {
          QFont font = static_cast<QWidget *>(QObject::parent())->font();
          font.setItalic(true);
          return font;
        }

        default:
          return QVariant();
      }
    }

    int row = index.row();
    QModelIndex parent = index.parent();
    RecentRepositories *repos = RecentRepositories::instance();
    switch (role) {
      case Qt::DisplayRole: {
        if (!parent.isValid()) {
          switch (row) {
            case Repo:
              return tr("open");
            case Recent:
              return tr("recent");
            case Remote:
              return tr("remote");
            default:
              return QVariant();
          }
        }

        switch (parent.row()) {
          case Repo:
            if (mTabs->count()) {
              if (!mShowFullPath)
                return mTabs->tabText(row);

              RepoView *view = static_cast<RepoView *>(mTabs->widget(row));
              return view->repo().workdir().path();
            }

            return tr("none");

          case Recent: {
            RecentRepositories *recent = RecentRepositories::instance();
            if (recent->count()) {
              RecentRepository *repo = repos->repository(row);
              return mShowFullPath ? repo->path() : repo->name();
            }

            return tr("none");
          }

          case Remote: {
            Accounts *accounts = Accounts::instance();
            if (accounts->count())
              return accounts->account(row)->username();
            return Account::name(static_cast<Account::Kind>(row));
          }

          default:
            return QVariant();
        }
      }

      case Qt::DecorationRole: {
        if (!parent.isValid())
          return QVariant();

        switch (parent.row()) {
          case Remote: {
            Accounts *accounts = Accounts::instance();
            if (accounts->count())
              return Account::icon(accounts->account(row)->kind());
            return Account::icon(static_cast<Account::Kind>(row));
          }

          default:
            return QVariant();
        }
      }

      case Qt::FontRole: {
        if (!parent.isValid()) {
          QFont font;
          font.setCapitalization(QFont::SmallCaps);
          font.setBold(true);
          return font;
        }

        switch (parent.row()) {
          case Repo: {
            if (mTabs->count())
              return QVariant();

            QFont font;
            font.setItalic(true);
            return font;
          }

          case Recent: {
            RecentRepositories *recent = RecentRepositories::instance();
            if (recent->count())
              return QVariant();

            QFont font;
            font.setItalic(true);
            return font;
          }

          default:
            return QVariant();
        }
      }

      case Qt::ForegroundRole:
        return QPalette().brush(!parent.isValid() ? QPalette::BrightText
                                                  : QPalette::Text);

      case Qt::SizeHintRole:
        return QSize(0, QFontMetrics(QFont()).lineSpacing() + 8);

      case PathRole: {
        if (!parent.isValid())
          return QVariant();

        switch (parent.row()) {
          case Repo: {
            if (!mTabs->count())
              return QVariant();
            QWidget *widget = mTabs->widget(row);
            RepoView *view = static_cast<RepoView *>(widget);
            return view->repo().workdir().path();
          }

          case Recent:
            if (!repos->count())
              return QVariant();
            return repos->repository(row)->path();

          default:
            return QVariant();
        }
      }

      case TabRole:
        return QVariant::fromValue(qobject_cast<RepoView *>(ptr));

      case RecentRole:
        return QVariant::fromValue(qobject_cast<RecentRepository *>(ptr));

      case AccountRole:
        return QVariant::fromValue(qobject_cast<Account *>(ptr));

      case AccountKindRole: {
        if (!parent.isValid())
          return QVariant();

        switch (parent.row()) {
          case Remote:
            if (Account *account = qobject_cast<Account *>(ptr))
              return account->kind();
            return static_cast<Account::Kind>(row);

          default:
            return QVariant();
        }
      }

      case Qt::ToolTipRole: {
        if (!parent.isValid() ||
            mShowFullPath) // makes no sense to show tooltip when path is
                           // already shown completely
          return "";

        switch (parent.row()) {
          case Repo:
            if (mTabs->count()) {
              RepoView *view = static_cast<RepoView *>(mTabs->widget(row));
              return view->repo().workdir().path();
            }

            return "";

          case Recent: {
            RecentRepositories *recent = RecentRepositories::instance();
            if (recent->count()) {
              RecentRepository *repo = repos->repository(row);
              return repo->path();
            }

            return "";
          }

          default:
            return QVariant();
        }
      }
      default:
        return QVariant();
    }
  }

  Qt::ItemFlags flags(const QModelIndex &index) const override {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.parent().isValid())
      flags &= ~Qt::ItemIsSelectable;
    return flags;
  }

  QModelIndex currentIndex() const {
    return index(mTabs->currentIndex(), 0, index(Repo, 0));
  }

  void setShowFullPath(bool enabled) {
    beginResetModel();
    mShowFullPath = enabled;
    endResetModel();
  }

  void setShowFullName(bool enabled) {
    beginResetModel();
    mShowFullName = enabled;
    endResetModel();
  }

private:
  TabWidget *mTabs;
  bool mShowFullPath = false;
  bool mShowFullName = false;

  QIcon mCloudIcon;
  QIcon mErrorIcon;
};

// Does this index correspond to one of the open repositories?
bool isRepoIndex(const QModelIndex &index) {
  QModelIndex parent = index.parent();
  return (parent.isValid() && !parent.parent().isValid() &&
          parent.row() == RepoModel::Repo);
}

bool isRemoteIndex(const QModelIndex &index) {
  QModelIndex parent = index.parent();
  return (parent.isValid() && !parent.parent().isValid() &&
          parent.row() == RepoModel::Remote);
}

void storeExpansionState(QTreeView *view) {
  QSettings settings;
  settings.beginGroup(kRemoteExpandedGroup);

  QAbstractItemModel *model = view->model();
  QModelIndex remote = model->index(RepoModel::Remote, 0);
  for (int i = 0; i < model->rowCount(remote); ++i) {
    QModelIndex index = model->index(i, 0, remote);
    QString key = index.data(Qt::DisplayRole).toString();
    settings.setValue(key, view->isExpanded(index));
  }

  settings.endGroup();
}

void restoreExpansionState(QTreeView *view) {
  QAbstractItemModel *model = view->model();
  for (int i = 0; i < model->rowCount(); ++i)
    view->expand(model->index(i, 0));

  QSettings settings;
  settings.beginGroup(kRemoteExpandedGroup);

  QModelIndex remote = model->index(RepoModel::Remote, 0);
  for (int i = 0; i < model->rowCount(remote); ++i) {
    QModelIndex index = model->index(i, 0, remote);
    QString key = index.data(Qt::DisplayRole).toString();
    view->setExpanded(index, settings.value(key, true).toBool());
  }

  settings.endGroup();
}

} // namespace

SideBar::SideBar(TabWidget *tabs, QWidget *parent) : QWidget(parent) {
  setStyleSheet(kStyleSheet);

  QTreeView *view = new QTreeView(this);
  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setContextMenuPolicy(Qt::CustomContextMenu);
  view->setFocusPolicy(Qt::NoFocus);
  view->setIconSize(QSize(20, 20));
  view->setRootIsDecorated(false);
  view->setHeaderHidden(true);

  view->setItemDelegate(new ProgressDelegate(view));

  RepoModel *model = new RepoModel(tabs, view);
  view->setModel(model);

  // Restore selection and expansion state after model reset.
  connect(model, &RepoModel::modelReset, view, [view, model] {
    view->setCurrentIndex(model->currentIndex());
    restoreExpansionState(view);
  });

  connect(tabs, &TabWidget::currentChanged, view,
          [view, model] { view->setCurrentIndex(model->currentIndex()); });

  // Store expansion state when it changes.
  connect(view, &QTreeView::collapsed, [view](const QModelIndex &index) {
    if (isRemoteIndex(index))
      storeExpansionState(view);
  });
  connect(view, &QTreeView::expanded, [view](const QModelIndex &index) {
    if (isRemoteIndex(index))
      storeExpansionState(view);
  });

  connect(view, &QTreeView::clicked, [tabs](const QModelIndex &index) {
    if (isRepoIndex(index))
      tabs->setCurrentIndex(index.row());
  });

  connect(
      view, &QTreeView::doubleClicked, [tabs, this](const QModelIndex &index) {
        if (isRepoIndex(index)) {
          tabs->setCurrentIndex(index.row());
          return;
        }

        // Open existing path.
        QString path = index.data(PathRole).toString();
        if (!path.isEmpty()) {
          MainWindow::open(path);
          return;
        }

        // Add remote account.
        QVariant accountKindVariant = index.data(AccountKindRole);
        if (accountKindVariant.isValid()) {
          Account *account = index.data(AccountRole).value<Account *>();
          AccountDialog *dialog = new AccountDialog(account, this);
          dialog->setKind(accountKindVariant.value<Account::Kind>());
          dialog->open();
          return;
        }

        // Clone remote repository.
        QVariant repoVariant = index.data(RepositoryRole);
        if (repoVariant.isValid()) {
          Repository *repo = repoVariant.value<Repository *>();
          CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, this, repo);
          connect(dialog, &CloneDialog::accepted, [repo, dialog] {
            // Set local path.
            Account *account = repo->account();
            account->setRepositoryPath(account->indexOf(repo), dialog->path());

            // Open the repo.
            MainWindow::open(dialog->path());
          });

          dialog->open();
        }
      });

  connect(view, &QTreeView::customContextMenuRequested,
          [this, view](const QPoint &point) {
            QMenu menu;
            QModelIndex index = view->indexAt(point);
            if (RepoView *view = index.data(TabRole).value<RepoView *>()) {
              menu.addAction(tr("Close"), view, &RepoView::close);
            } else if (Account *account =
                           index.data(AccountRole).value<Account *>()) {
              menu.addAction(tr("Remove"), [this, account] {
                promptToRemoveAccount(account);
              });

              if (account->isAuthorizeSupported())
                menu.addAction(tr("Authorize"), account, &Account::authorize);
            }

            if (!menu.isEmpty())
              menu.exec(view->mapToGlobal(point));
          });

  // footer
  Footer *footer = new Footer(this);

  // plus button
  QMenu *plusMenu = new QMenu(this);
  footer->setPlusMenu(plusMenu);

  QAction *clone = plusMenu->addAction(tr("Clone Repository"));
  connect(clone, &QAction::triggered, [this] {
    CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, this);
    connect(dialog, &CloneDialog::accepted, [dialog] {
      if (MainWindow *window = MainWindow::open(dialog->path()))
        window->currentView()->addLogEntry(dialog->message(),
                                           dialog->messageTitle());
    });
    dialog->open();
  });

  QAction *open = plusMenu->addAction(tr("Open Existing Repository"));
  connect(open, &QAction::triggered, [this] {
    // FIXME: Filter out non-git dirs.
    QFileDialog *dialog =
        new QFileDialog(this, tr("Open Repository"), QDir::homePath());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly);
    connect(dialog, &QFileDialog::fileSelected,
            [](const QString &path) { MainWindow::open(path); });
    dialog->open();
  });

  QAction *init = plusMenu->addAction(tr("Initialize New Repository"));
  connect(init, &QAction::triggered, [this] {
    CloneDialog *dialog = new CloneDialog(CloneDialog::Init, this);
    connect(dialog, &CloneDialog::accepted, [dialog] {
      if (MainWindow *window = MainWindow::open(dialog->path()))
        window->currentView()->addLogEntry(dialog->message(),
                                           dialog->messageTitle());
    });
    dialog->open();
  });

  plusMenu->addSeparator();

  for (int i = 0; i < Account::NUM_KINDS; ++i) {
    Account::Kind kind = static_cast<Account::Kind>(i);
    QString text = tr("Add %1 Account").arg(Account::name(kind));
    QAction *add = plusMenu->addAction(text);
    connect(add, &QAction::triggered, [this, kind] {
      AccountDialog *dialog = new AccountDialog(nullptr, this);
      dialog->setKind(kind);
      dialog->open();
    });
  }

  // minus button
  QItemSelectionModel *sel = view->selectionModel();
  connect(sel, &QItemSelectionModel::selectionChanged, [footer, sel] {
    QModelIndexList indexes = sel->selectedIndexes();
    if (indexes.isEmpty())
      return;

    QModelIndex index = indexes.first();
    QString path = index.data(PathRole).toString();
    Account *account = index.data(AccountRole).value<Account *>();
    footer->setMinusEnabled(!path.isEmpty() || account);
  });

  connect(footer, &Footer::minusClicked, [this, view, model, sel] {
    foreach (const QModelIndex &index, sel->selectedIndexes()) {
      if (RepoView *tab = index.data(TabRole).value<RepoView *>()) {
        tab->close();

      } else if (index.data(RecentRole).value<RecentRepository *>()) {
        RecentRepositories::instance()->remove(index.row());

      } else if (auto account = index.data(AccountRole).value<Account *>()) {
        promptToRemoveAccount(account);

      } else if (auto repo = index.data(RepositoryRole).value<Repository *>()) {
        QString fmt =
            tr("<p>Are you sure you want to remove the remote repository "
               "association for %1?</p><p>The local clone itself will not "
               "be affected.</p>");

        QMessageBox *dialog = new QMessageBox(
            QMessageBox::Warning, tr("Remove Repository Association?"),
            fmt.arg(repo->fullName()), QMessageBox::Cancel, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        QPushButton *remove =
            dialog->addButton(tr("Remove"), QMessageBox::AcceptRole);
        remove->setFocus();
        connect(remove, &QPushButton::clicked, [index, repo] {
          repo->account()->setRepositoryPath(index.row(), QString());
        });

        dialog->open();
      }
    }

    // Reset selection to current tab.
    view->setCurrentIndex(model->currentIndex());
  });

  // context menu
  QSettings settings;
  QMenu *contextMenu = new QMenu(this);
  footer->setContextMenu(contextMenu);

  QAction *clear = contextMenu->addAction(tr("Clear All Recent"));
  connect(clear, &QAction::triggered,
          [] { RecentRepositories::instance()->clear(); });

  QAction *showFullPath = contextMenu->addAction(tr("Show Full Path"));
  bool recentChecked = settings.value("start/recent/fullpath").toBool();
  showFullPath->setCheckable(true);
  showFullPath->setChecked(recentChecked);
  model->setShowFullPath(recentChecked);
  connect(showFullPath, &QAction::triggered, [model](bool checked) {
    QSettings().setValue("start/recent/fullpath", checked);
    model->setShowFullPath(checked);
  });

  QAction *filter = contextMenu->addAction(tr("Filter Non-existent Paths"));
  filter->setCheckable(true);
  filter->setChecked(settings.value("recent/filter", true).toBool());
  connect(filter, &QAction::triggered,
          [](bool checked) { QSettings().setValue("recent/filter", checked); });

  contextMenu->addSeparator();

  QAction *refresh = contextMenu->addAction(tr("Refresh Remote Accounts"));
  connect(refresh, &QAction::triggered, [] {
    Accounts *accounts = Accounts::instance();
    for (int i = 0; i < accounts->count(); ++i)
      accounts->account(i)->connect();
  });

  QAction *showFullName = contextMenu->addAction(tr("Show Full Name"));
  bool remoteChecked = settings.value("start/remote/fullname").toBool();
  showFullName->setCheckable(true);
  showFullName->setChecked(remoteChecked);
  model->setShowFullName(remoteChecked);
  connect(showFullName, &QAction::triggered, [model](bool checked) {
    QSettings().setValue("start/remote/fullname", checked);
    model->setShowFullName(checked);
  });

  // Create layout.
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(view);
  layout->addWidget(footer);

  // Disable footer resize.
  footer->setMinimumWidth(footer->sizeHint().width());
}

QSize SideBar::sizeHint() const { return QSize(192, 0); }

QSize SideBar::minimumSizeHint() const { return QSize(0, 0); }

void SideBar::promptToRemoveAccount(Account *account) {
  QString fmt =
      tr("<p>Are you sure you want to remove the %1 account for '%2'?</p>"
         "<p>Only the account association will be removed. Remote "
         "configurations and local clones will not be affected.</p>");

  QMessageBox *dialog = new QMessageBox(
      QMessageBox::Warning, tr("Remove Account?"),
      fmt.arg(Account::name(account->kind()), account->username()),
      QMessageBox::Cancel, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);

  QPushButton *remove =
      dialog->addButton(tr("Remove"), QMessageBox::AcceptRole);
  remove->setFocus();
  connect(remove, &QPushButton::clicked,
          [account] { Accounts::instance()->removeAccount(account); });

  dialog->open();
}

#include "SideBar.moc"
