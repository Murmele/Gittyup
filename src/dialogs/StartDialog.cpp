//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "StartDialog.h"
#include "AccountDialog.h"
#include "CloneDialog.h"
#include "IconLabel.h"
#include "app/Application.h"
#include "conf/RecentRepositories.h"
#include "conf/RecentRepository.h"
#include "host/Accounts.h"
#include "host/Repository.h"
#include "ui/Footer.h"
#include "ui/MainWindow.h"
#include "ui/ProgressIndicator.h"
#include "ui/RepoView.h"
#include "ui/TabWidget.h"
#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QMenu>
#include <QMultiMap>
#include <QPushButton>
#include <QPointer>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

const QString kSubtitleFmt = "<h4 style='margin-top: 0px; color: gray'>%2</h4>";

const QString kGeometryKey = "geometry";
const QString kStartGroup = "start";

const QString kCloudIcon = ":/cloud.png";

enum Role { KindRole = Qt::UserRole, AccountRole, RepositoryRole };

class RepoModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Row { Clone, Open, Init };

  RepoModel(QObject *parent = nullptr) : QAbstractListModel(parent) {
    RecentRepositories *repos = RecentRepositories::instance();
    connect(repos, &RecentRepositories::repositoryAboutToBeAdded, this,
            &RepoModel::beginResetModel);
    connect(repos, &RecentRepositories::repositoryAdded, this,
            &RepoModel::endResetModel);
    connect(repos, &RecentRepositories::repositoryAboutToBeRemoved, this,
            &RepoModel::beginResetModel);
    connect(repos, &RecentRepositories::repositoryRemoved, this,
            &RepoModel::endResetModel);
  }

  void setShowFullPath(bool enabled) {
    beginResetModel();
    mShowFullPath = enabled;
    endResetModel();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    RecentRepositories *repos = RecentRepositories::instance();
    return (repos->count() > 0) ? repos->count() : 3;
  }

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    RecentRepositories *repos = RecentRepositories::instance();
    if (repos->count() <= 0) {
      switch (role) {
        case Qt::DisplayRole:
          switch (index.row()) {
            case Clone:
              return tr("Clone Repository");
            case Open:
              return tr("Open Existing Repository");
            case Init:
              return tr("Initialize New Repository");
          }

        case Qt::DecorationRole: {
          switch (index.row()) {
            case Clone:
              return QIcon(":/clone.png");
            case Open:
              return QIcon(":/open.png");
            case Init:
              return QIcon(":/new.png");
          }
        }
      }

      return QVariant();
    }

    RecentRepository *repo = repos->repository(index.row());
    switch (role) {
      case Qt::DisplayRole:
        return mShowFullPath ? repo->gitpath() : repo->name();

      case Qt::UserRole:
        return repo->gitpath();
    }

    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex &index) const override {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (RecentRepositories::instance()->count() <= 0)
      flags &= ~Qt::ItemIsSelectable;

    return flags;
  }

private:
  bool mShowFullPath = false;
};

class HostModel : public QAbstractItemModel {
  Q_OBJECT

public:
  HostModel(QStyle *style, QObject *parent = nullptr)
      : QAbstractItemModel(parent),
        mErrorIcon(style->standardIcon(QStyle::SP_MessageBoxCritical)) {
    Accounts *accounts = Accounts::instance();
    connect(accounts, &Accounts::accountAboutToBeAdded, this,
            &HostModel::beginResetModel);
    connect(accounts, &Accounts::accountAdded, this, &HostModel::endResetModel);
    connect(accounts, &Accounts::accountAboutToBeRemoved, this,
            &HostModel::beginResetModel);
    connect(accounts, &Accounts::accountRemoved, this,
            &HostModel::endResetModel);

    connect(accounts, &Accounts::progress, this, [this](int accountIndex) {
      QModelIndex idx = index(0, 0, index(accountIndex, 0));
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
            &HostModel::beginResetModel);
    connect(accounts, &Accounts::repositoryAdded, this,
            &HostModel::endResetModel);
    connect(accounts, &Accounts::repositoryPathChanged, this,
            [this](int accountIndex, int repoIndex) {
              QModelIndex idx = index(repoIndex, 0, index(accountIndex, 0));
              emit dataChanged(idx, idx, {Qt::DisplayRole});
            });
  }

  void setShowFullName(bool enabled) {
    beginResetModel();
    mShowFullName = enabled;
    endResetModel();
  }

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override {
    bool id = (!parent.isValid() || parent.internalId());
    return createIndex(row, column, !id ? parent.row() + 1 : 0);
  }

  QModelIndex parent(const QModelIndex &index) const override {
    quintptr id = index.internalId();
    return !id ? QModelIndex() : createIndex(id - 1, 0);
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    // no accounts
    Accounts *accounts = Accounts::instance();
    if (accounts->count() <= 0)
      return !parent.isValid() ? 4 : 0;

    // account
    if (!parent.isValid())
      return accounts->count();

    if (parent.internalId())
      return 0;

    // repos
    Account *account = accounts->account(parent.row());
    int count = account->repositoryCount();
    if (count > 0)
      return count;

    AccountError *error = account->error();
    AccountProgress *progress = account->progress();
    return (error->isValid() || progress->isValid()) ? 1 : 0;
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const override {
    return 1;
  }

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    // no accounts
    Accounts *accounts = Accounts::instance();
    if (accounts->count() <= 0) {
      Account::Kind kind = static_cast<Account::Kind>(index.row());
      switch (role) {
        case Qt::DisplayRole:
          return Account::name(kind);
        case Qt::DecorationRole:
          return Account::icon(kind);
        case KindRole:
          return kind;
      }

      return QVariant();
    }

    // account
    quintptr id = index.internalId();
    if (!id) {
      Account *account = accounts->account(index.row());
      switch (role) {
        case Qt::DisplayRole:
          return account->username();
        case Qt::DecorationRole:
          return Account::icon(account->kind());
        case KindRole:
          return account->kind();
        case AccountRole:
          return QVariant::fromValue(account);
      }

      return QVariant();
    }

    // error
    Account *account = accounts->account(id - 1);
    AccountError *error = account->error();
    if (error->isValid()) {
      switch (role) {
        case Qt::DisplayRole:
          return error->text();
        case Qt::DecorationRole:
          return mErrorIcon;
        case Qt::ToolTipRole:
          return error->detailedText();
      }

      return QVariant();
    }

    // progress
    if (account->repositoryCount() <= 0) {
      switch (role) {
        case Qt::DisplayRole:
          return tr("Connecting");

        case Qt::DecorationRole:
          return account->progress()->value();

        case Qt::FontRole: {
          QFont font = static_cast<QWidget *>(QObject::parent())->font();
          font.setItalic(true);
          return font;
        }

        default:
          return QVariant();
      }
    }

    // repo
    int row = index.row();
    Repository *repo = account->repository(row);
    switch (role) {
      case Qt::DisplayRole:
        return mShowFullName ? repo->fullName() : repo->name();

      case Qt::DecorationRole: {
        QString path = account->repositoryPath(row);
        return path.isEmpty() ? QIcon(kCloudIcon) : QIcon();
      }

      case KindRole:
        return account->kind();
      case RepositoryRole:
        return QVariant::fromValue(repo);
    }

    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex &index) const override {
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (Accounts::instance()->count() <= 0)
      flags &= ~Qt::ItemIsSelectable;
    return flags;
  }

private:
  bool mShowFullName = false;
  QIcon mErrorIcon;
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

} // namespace

StartDialog::StartDialog(QWidget *parent) : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Choose Repository"));

  QIcon icon(":/Gittyup.iconset/icon_128x128.png");
  IconLabel *iconLabel = new IconLabel(icon, 128, 128, this);

  QIcon title(":/logo-type_light@2x.png");
  IconLabel *titleLabel = new IconLabel(title, 163, 38, this);

  QString subtitleText = kSubtitleFmt.arg(tr("Understand your history!"));
  QLabel *subtitle = new QLabel(subtitleText, this);
  subtitle->setAlignment(Qt::AlignHCenter);

  QVBoxLayout *left = new QVBoxLayout;
  left->addWidget(iconLabel);
  left->addWidget(titleLabel);
  left->addWidget(subtitle);
  left->addStretch();

  mRepoList = new QListView(this);
  mRepoList->setIconSize(QSize(32, 32));
  mRepoList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(mRepoList, &QListView::clicked, this,
          [this](const QModelIndex &index) {
            if (!index.data(Qt::UserRole).isValid()) {
              switch (index.row()) {
                case RepoModel::Clone:
                  mClone->trigger();
                  break;
                case RepoModel::Open:
                  mOpen->trigger();
                  break;
                case RepoModel::Init:
                  mInit->trigger();
                  break;
              }
            }
          });

  connect(mRepoList, &QListView::doubleClicked, this, &QDialog::accept);

  RepoModel *repoModel = new RepoModel(mRepoList);
  mRepoList->setModel(repoModel);
  connect(repoModel, &RepoModel::modelReset, this, [this] {
    mRepoList->setCurrentIndex(mRepoList->model()->index(0, 0));
  });

  mRepoFooter = new Footer(mRepoList);
  connect(mRepoFooter, &Footer::minusClicked, this, [this] {
    // Sort selection in reverse order.
    QModelIndexList indexes = mRepoList->selectionModel()->selectedIndexes();
    std::sort(indexes.begin(), indexes.end(),
              [](const QModelIndex &lhs, const QModelIndex &rhs) {
                return rhs.row() < lhs.row();
              });

    // Remove selected indexes from settings.
    foreach (const QModelIndex &index, indexes)
      RecentRepositories::instance()->remove(index.row());
  });

  QMenu *repoPlusMenu = new QMenu(this);
  mRepoFooter->setPlusMenu(repoPlusMenu);

  mClone = repoPlusMenu->addAction(tr("Clone Repository"));
  connect(mClone, &QAction::triggered, this, [this] {
    CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, this);
    connect(dialog, &CloneDialog::accepted, this, [this, dialog] {
      if (MainWindow *window = openWindow(dialog->path()))
        window->currentView()->addLogEntry(dialog->message(),
                                           dialog->messageTitle());
    });
    dialog->open();
  });

  mOpen = repoPlusMenu->addAction(tr("Open Existing Repository"));
  connect(mOpen, &QAction::triggered, this, [this] {
    // FIXME: Filter out non-git dirs.
    QFileDialog *dialog =
        new QFileDialog(this, tr("Open Repository"), QDir::homePath());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly);
    connect(dialog, &QFileDialog::fileSelected, this, &StartDialog::openWindow);
    dialog->open();
  });

  mInit = repoPlusMenu->addAction(tr("Initialize New Repository"));
  connect(mInit, &QAction::triggered, this, [this] {
    CloneDialog *dialog = new CloneDialog(CloneDialog::Init, this);
    connect(dialog, &CloneDialog::accepted, this, [this, dialog] {
      if (MainWindow *window = openWindow(dialog->path()))
        window->currentView()->addLogEntry(dialog->message(),
                                           dialog->messageTitle());
    });
    dialog->open();
  });

  QMenu *repoContextMenu = new QMenu(this);
  mRepoFooter->setContextMenu(repoContextMenu);

  QAction *clear = repoContextMenu->addAction(tr("Clear All"));
  connect(clear, &QAction::triggered,
          [] { RecentRepositories::instance()->clear(); });

  QSettings settings;
  QAction *showFullPath = repoContextMenu->addAction(tr("Show Full Path"));
  bool recentChecked = settings.value("start/recent/fullpath").toBool();
  showFullPath->setCheckable(true);
  showFullPath->setChecked(recentChecked);
  repoModel->setShowFullPath(recentChecked);
  connect(showFullPath, &QAction::triggered, this, [repoModel](bool checked) {
    QSettings().setValue("start/recent/fullpath", checked);
    repoModel->setShowFullPath(checked);
  });

  QAction *filter = repoContextMenu->addAction(tr("Filter Non-existent Paths"));
  filter->setCheckable(true);
  filter->setChecked(settings.value("recent/filter", true).toBool());
  connect(filter, &QAction::triggered,
          [](bool checked) { QSettings().setValue("recent/filter", checked); });

  QVBoxLayout *middle = new QVBoxLayout;
  middle->setSpacing(0);
  middle->addWidget(new QLabel(tr("Repositories:"), this));
  middle->addSpacing(8); // FIXME: Query style?
  middle->addWidget(mRepoList);
  middle->addWidget(mRepoFooter);

  mHostTree = new QTreeView(this);
  mHostTree->setHeaderHidden(true);
  mHostTree->setExpandsOnDoubleClick(false);
  mHostTree->setIconSize(QSize(32, 32));
  mHostTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(mHostTree, &QTreeView::clicked, this,
          [this](const QModelIndex &index) {
            int rows = mHostTree->model()->rowCount(index);
            if (!rows && !index.data(RepositoryRole).isValid())
              edit(index);
          });

  connect(mHostTree, &QTreeView::doubleClicked, this,
          [this](const QModelIndex &index) {
            QModelIndex parent = index.parent();
            if (parent.isValid()) {
              Account *account = parent.data(AccountRole).value<Account *>();
              if (!account->repositoryPath(index.row()).isEmpty()) {
                accept();
                return;
              }
            }

            edit(index);
          });

  HostModel *hostModel = new HostModel(style(), mHostTree);
  mHostTree->setModel(hostModel);
  connect(hostModel, &QAbstractItemModel::modelReset, this, [this] {
    QModelIndex index = mHostTree->model()->index(0, 0);
    mHostTree->setRootIsDecorated(index.data(AccountRole).isValid());
    mHostTree->expandAll();
  });

  mHostTree->setItemDelegate(new ProgressDelegate(this));

  mHostFooter = new Footer(mHostTree);
  connect(mHostFooter, &Footer::plusClicked, this, [this] { edit(); });
  connect(mHostFooter, &Footer::minusClicked, this, &StartDialog::remove);

  QMenu *hostContextMenu = new QMenu(this);
  mHostFooter->setContextMenu(hostContextMenu);

  QAction *refresh = hostContextMenu->addAction(tr("Refresh"));
  connect(refresh, &QAction::triggered, this, [] {
    Accounts *accounts = Accounts::instance();
    for (int i = 0; i < accounts->count(); ++i)
      accounts->account(i)->connect();
  });

  QAction *showFullName = hostContextMenu->addAction(tr("Show Full Name"));
  bool remoteChecked = QSettings().value("start/remote/fullname").toBool();
  showFullName->setCheckable(true);
  showFullName->setChecked(remoteChecked);
  hostModel->setShowFullName(remoteChecked);
  connect(showFullName, &QAction::triggered, this, [hostModel](bool checked) {
    QSettings().setValue("start/remote/fullname", checked);
    hostModel->setShowFullName(checked);
  });

  // Clear the other list when this selection changes.
  QItemSelectionModel *repoSelModel = mRepoList->selectionModel();
  connect(repoSelModel, &QItemSelectionModel::selectionChanged, this, [this] {
    if (!mRepoList->selectionModel()->selectedIndexes().isEmpty())
      mHostTree->clearSelection();
    updateButtons();
  });

  // Clear the other list when this selection changes.
  QItemSelectionModel *hostSelModel = mHostTree->selectionModel();
  connect(hostSelModel, &QItemSelectionModel::selectionChanged, this, [this] {
    if (!mHostTree->selectionModel()->selectedIndexes().isEmpty())
      mRepoList->clearSelection();
    updateButtons();
  });

  QVBoxLayout *right = new QVBoxLayout;
  right->setSpacing(0);
  right->addWidget(new QLabel(tr("Remote:"), this));
  right->addSpacing(8); // FIXME: Query style?
  right->addWidget(mHostTree);
  right->addWidget(mHostFooter);

  QHBoxLayout *top = new QHBoxLayout;
  top->addLayout(left);
  top->addSpacing(12);
  top->addLayout(middle);
  top->addSpacing(12);
  top->addLayout(right);

  QDialogButtonBox::StandardButtons buttons =
      QDialogButtonBox::Open | QDialogButtonBox::Cancel;
  mButtonBox = new QDialogButtonBox(buttons, this);
  connect(mButtonBox, &QDialogButtonBox::accepted, this, &StartDialog::accept);
  connect(mButtonBox, &QDialogButtonBox::rejected, this, &StartDialog::reject);

  QString text = tr("View Getting Started Video");
  QPushButton *help = mButtonBox->addButton(text, QDialogButtonBox::ResetRole);
  connect(help, &QPushButton::clicked, [] {
    QDesktopServices::openUrl(QUrl("https://gitahead.com/#tutorials"));
  });

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(top);
  layout->addWidget(mButtonBox);
}

void StartDialog::accept() {
  QModelIndexList repoIndexes = mRepoList->selectionModel()->selectedIndexes();
  QModelIndexList hostIndexes = mHostTree->selectionModel()->selectedIndexes();

  QStringList paths;
  foreach (const QModelIndex &index, repoIndexes)
    paths.append(index.data(Qt::UserRole).toString());

  QModelIndexList uncloned;
  foreach (const QModelIndex &index, hostIndexes) {
    QModelIndex parent = index.parent();
    if (parent.isValid()) {
      Account *account = parent.data(AccountRole).value<Account *>();
      QString path = account->repositoryPath(index.row());
      if (path.isEmpty()) {
        uncloned.append(index);
      } else {
        paths.append(path);
      }
    }
  }

  // Clone the first repo.
  if (!uncloned.isEmpty()) {
    edit(uncloned.first());
    return;
  }

  // FIXME: Fail if none of the windows were able to be opened?
  QDialog::accept();

  if (paths.isEmpty())
    return;

  // Open a new window for the first valid repo.
  MainWindow *window = MainWindow::open(paths.takeFirst());
  while (!window && !paths.isEmpty())
    window = MainWindow::open(paths.takeFirst());

  if (!window)
    return;

  // Add the remainder as tabs.
  foreach (const QString &path, paths)
    window->addTab(path);
}

StartDialog *StartDialog::openSharedInstance() {
  static QPointer<StartDialog> dialog;
  if (dialog) {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    return dialog;
  }

  dialog = new StartDialog;
  dialog->show();
  return dialog;
}

void StartDialog::showEvent(QShowEvent *event) {
  // Restore geometry.
  QSettings settings;
  settings.beginGroup(kStartGroup);
  if (settings.contains(kGeometryKey))
    restoreGeometry(settings.value(kGeometryKey).toByteArray());
  settings.endGroup();

  QDialog::showEvent(event);
}

void StartDialog::hideEvent(QHideEvent *event) {
  QSettings settings;
  settings.beginGroup(kStartGroup);
  settings.setValue(kGeometryKey, saveGeometry());
  settings.endGroup();

  QDialog::hideEvent(event);
}

void StartDialog::updateButtons() {
  QModelIndexList repoIndexes = mRepoList->selectionModel()->selectedIndexes();
  QModelIndexList hostIndexes = mHostTree->selectionModel()->selectedIndexes();

  // Update dialog Open button.
  bool clone = false;
  QPushButton *open = mButtonBox->button(QDialogButtonBox::Open);
  open->setEnabled(!repoIndexes.isEmpty() || !hostIndexes.isEmpty());
  foreach (const QModelIndex &index, hostIndexes) {
    QModelIndex parent = index.parent();
    if (!parent.isValid()) {
      open->setEnabled(false);
    } else {
      Account *account = parent.data(AccountRole).value<Account *>();
      if (nullptr != index.data(RepositoryRole).value<Repository *>()) {
        if (account->repositoryPath(index.row()).isEmpty())
          clone = true;
      } else {
        open->setEnabled(false);
      }
    }
  }

  open->setText((clone && open->isEnabled()) ? tr("Clone") : tr("Open"));

  // Update repo list footer buttons.
  mRepoFooter->setMinusEnabled(!repoIndexes.isEmpty());

  // Update host list footer buttons.
  mHostFooter->setMinusEnabled(false);
  if (!hostIndexes.isEmpty()) {
    QModelIndex index = hostIndexes.first();
    Account *account = index.data(AccountRole).value<Account *>();

    QString repoPath;
    QModelIndex parent = index.parent();
    if (parent.isValid()) {
      Account *account = parent.data(AccountRole).value<Account *>();
      repoPath = account->repositoryPath(index.row());
    }

    mHostFooter->setMinusEnabled(account || !repoPath.isEmpty());
  }
}

void StartDialog::edit(const QModelIndex &index) {
  // account
  if (!index.isValid() || !index.parent().isValid()) {
    Account *account = nullptr;
    if (index.isValid())
      account = index.data(AccountRole).value<Account *>();

    AccountDialog *dialog = new AccountDialog(account, this);
    if (!account && index.isValid())
      dialog->setKind(index.data(KindRole).value<Account::Kind>());

    dialog->open();
    return;
  }

  // repo
  Repository *repo = index.data(RepositoryRole).value<Repository *>();
  if (!repo)
    return;

  CloneDialog *dialog = new CloneDialog(CloneDialog::Clone, this, repo);
  connect(dialog, &CloneDialog::accepted, this, [this, index, dialog] {
    // Set local path.
    Account *account = Accounts::instance()->account(index.parent().row());
    account->setRepositoryPath(index.row(), dialog->path());

    // Open the repo.
    QCoreApplication::processEvents();
    accept();
  });

  dialog->open();
}

void StartDialog::remove() {
  QModelIndexList indexes = mHostTree->selectionModel()->selectedIndexes();
  Q_ASSERT(!indexes.isEmpty());

  // account
  QModelIndex index = indexes.first();
  if (!index.parent().isValid()) {
    Account::Kind kind = index.data(KindRole).value<Account::Kind>();
    QString name = index.data(Qt::DisplayRole).toString();
    QString fmt =
        tr("<p>Are you sure you want to remove the %1 account for '%2'?</p>"
           "<p>Only the account association will be removed. Remote "
           "configurations and local clones will not be affected.</p>");

    QMessageBox mb(QMessageBox::Warning, tr("Remove Account?"),
                   fmt.arg(Account::name(kind), name), QMessageBox::Cancel,
                   this);
    QPushButton *remove = mb.addButton(tr("Remove"), QMessageBox::AcceptRole);
    remove->setFocus();

    mb.exec();
    if (mb.clickedButton() == remove) {
      mHostTree->clearSelection();
      Accounts::instance()->removeAccount(index.row());
    }

    return;
  }

  // repo
  Repository *repo = index.data(RepositoryRole).value<Repository *>();
  QString fmt =
      tr("<p>Are you sure you want to remove the remote repository association "
         "for %1?</p><p>The local clone itself will not be affected.</p>");

  QMessageBox mb(QMessageBox::Warning, tr("Remove Repository Association?"),
                 fmt.arg(repo->fullName()), QMessageBox::Cancel, this);
  QPushButton *remove = mb.addButton(tr("Remove"), QMessageBox::AcceptRole);
  remove->setFocus();

  mb.exec();
  if (mb.clickedButton() == remove)
    repo->account()->setRepositoryPath(index.row(), QString());

  // Update minus and OK buttons.
  updateButtons();
}

MainWindow *StartDialog::openWindow(const QString &repo) {
  // This dialog has to be hidden before opening another window so that it
  // doesn't automatically move to a position that still shows the dialog.
  hide();
  if (MainWindow *window = MainWindow::open(repo)) {
    // Dismiss this dialog without trying to open the selection.
    reject();
    return window;
  } else {
    show();
  }

  return nullptr;
}

#include "StartDialog.moc"
