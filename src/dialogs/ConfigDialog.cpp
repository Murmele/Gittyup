//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#include "ConfigDialog.h"
#include "AddRemoteDialog.h"
#include "BranchDelegate.h"
#include "BranchTableModel.h"
#include "DeleteBranchDialog.h"
#include "DiffPanel.h"
#include "NewBranchDialog.h"
#include "PluginsPanel.h"
#include "RemoteTableModel.h"
#include "SubmoduleDelegate.h"
#include "SubmoduleTableModel.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include "git/Reference.h"
#include "git/Submodule.h"
#include "index/Index.h"
#include "ui/BlameEditor.h"
#include "ui/EditorWindow.h"
#include "ui/Footer.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QAction>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringListModel>
#include <QTableView>
#include <QTextEdit>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {

class StackedWidget : public QStackedWidget
{
public:
  StackedWidget(QWidget *parent = nullptr)
    : QStackedWidget(parent)
  {}

  QSize sizeHint() const override
  {
    return currentWidget()->sizeHint();
  }

  QSize minimumSizeHint() const override
  {
    return currentWidget()->minimumSizeHint();
  }
};

class GeneralPanel : public QWidget
{
  Q_OBJECT

public:
  GeneralPanel(RepoView *view, QWidget *parent = nullptr)
    : QWidget(parent), mRepo(view->repo())
  {
    mName = new QLineEdit(this);
    mEmail = new QLineEdit(this);

    mFetch = new QCheckBox(tr("Fetch every"), this);
    mFetchMinutes = new QSpinBox(this);
    connect(mFetch, &QCheckBox::toggled,
            mFetchMinutes, &QSpinBox::setEnabled);

    QHBoxLayout *fetchLayout = new QHBoxLayout;
    fetchLayout->addWidget(mFetch);
    fetchLayout->addWidget(mFetchMinutes);
    fetchLayout->addWidget(new QLabel(tr("minutes"), this));
    fetchLayout->addStretch();

    mPushCommit = new QCheckBox(tr("Push after each commit"), this);
    mPullUpdate = new QCheckBox(tr("Update submodules after pull"), this);
    mAutoPrune = new QCheckBox(tr("Prune when fetching"), this);

    QFormLayout *form = new QFormLayout(this);
    form->addRow(tr("User name:"), mName);
    form->addRow(tr("User email:"), mEmail);
    form->addRow(tr("Automatic actions:"), fetchLayout);
    form->addRow(QString(), mPushCommit);
    form->addRow(QString(), mPullUpdate);
    form->addRow(QString(), mAutoPrune);

    init();

    // Connect signals after initializing fields.
    connect(mName, &QLineEdit::textChanged, this, [this](const QString &text) {
      git::Config config = mRepo.config();
      config.setValue("user.name", text);
    });

    connect(mEmail, &QLineEdit::textChanged, this, [this](const QString &text) {
      git::Config config = mRepo.config();
      config.setValue("user.email", text);
    });

    connect(mFetch, &QCheckBox::toggled, view, [this, view](bool checked) {
      git::Config config = mRepo.config();
      config.setValue("autofetch.enable", checked);
      view->startFetchTimer();
    });

    using Signal = void (QSpinBox::*)(int);
    auto signal = static_cast<Signal>(&QSpinBox::valueChanged);
    connect(mFetchMinutes, signal, this, [this](int value) {
      git::Config config = mRepo.config();
      config.setValue("autofetch.minutes", value);
    });

    connect(mPushCommit, &QCheckBox::toggled, this, [this](bool checked) {
      git::Config config = mRepo.config();
      config.setValue("autopush.enable", checked);
    });

    connect(mPullUpdate, &QCheckBox::toggled, this, [this](bool checked) {
      git::Config config = mRepo.config();
      config.setValue("autoupdate.enable", checked);
    });

    connect(mAutoPrune, &QCheckBox::toggled, this, [this](bool checked) {
      git::Config config = mRepo.config();
      config.setValue("autoprune.enable", checked);
    });
  }

  void init()
  {
    git::Config config = mRepo.config();
    mName->setText(config.value<QString>("user.name"));
    mEmail->setText(config.value<QString>("user.email"));

    // Read defaults from global settings.
    Settings *settings = Settings::instance();
    settings->beginGroup("global");
    settings->beginGroup("autofetch");
    bool fetch = settings->value("enable").toBool();
    int minutes = settings->value("minutes").toInt();
    settings->endGroup();

    bool push = settings->value("autopush/enable").toBool();
    bool update = settings->value("autoupdate/enable").toBool();
    bool prune = settings->value("autoprune/enable").toBool();
    settings->endGroup();

    git::Config app = mRepo.appConfig();
    mFetch->setChecked(app.value<bool>("autofetch.enable", fetch));
    mFetchMinutes->setValue(app.value<int>("autofetch.minutes", minutes));
    mPushCommit->setChecked(app.value<bool>("autopush.enable", push));
    mPullUpdate->setChecked(app.value<bool>("autoupdate.enable", update));
    mAutoPrune->setChecked(app.value<bool>("autoprune.enable", prune));
  }

private:
  git::Repository mRepo;
  QLineEdit *mName;
  QLineEdit *mEmail;

  QCheckBox *mFetch;
  QSpinBox *mFetchMinutes;
  QCheckBox *mPushCommit;
  QCheckBox *mPullUpdate;
  QCheckBox *mAutoPrune;
};

class RemotesPanel : public QWidget
{
  Q_OBJECT

public:
  RemotesPanel(const git::Repository &repo, QWidget *parent = nullptr)
    : QWidget(parent), mRepo(repo)
  {
    QTableView *table = new QTableView(this);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::SelectedClicked);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setShowGrid(false);

    table->setModel(new RemoteTableModel(this, repo));

    Footer *footer = new Footer(table);
    connect(footer, &Footer::plusClicked, this, [this] {
      addRemote();
    });

    connect(footer, &Footer::minusClicked, this, [this, table] {
      QModelIndexList indexes = table->selectionModel()->selectedRows();
      foreach (const QModelIndex &index, indexes) {
        QString name = index.data().toString();
        QString title = tr("Delete Remote?");
        QString text = tr("Are you sure you want to delete '%1'?");
        QMessageBox msg(QMessageBox::Warning, title, text.arg(name),
                        QMessageBox::Cancel, this);

        QPushButton *remove =
          msg.addButton(tr("Delete"), QMessageBox::AcceptRole);

        msg.exec();

        if (msg.clickedButton() == remove)
          mRepo.deleteRemote(name);
      }
    });

    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
    [table, footer] {
      QModelIndexList indexes = table->selectionModel()->selectedRows();
      footer->setMinusEnabled(!indexes.isEmpty());
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->addWidget(table);
    layout->addWidget(footer);
  }

  void addRemote(const QString &name = QString())
  {
    AddRemoteDialog *dialog = new AddRemoteDialog(name, this);
    connect(dialog, &QDialog::accepted, this, [this, dialog] {
      mRepo.addRemote(dialog->name(), dialog->url());
    });

    dialog->open();
  }

private:
  git::Repository mRepo;
};

class BranchesPanel : public QWidget
{
public:
  BranchesPanel(const git::Repository &repo, QWidget *parent = nullptr)
    : QWidget(parent)
  {
    mTable = new QTableView(this);
    mTable->verticalHeader()->setVisible(false);
    mTable->setEditTriggers(QAbstractItemView::SelectedClicked);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTable->setShowGrid(false);

    mTable->setModel(new BranchTableModel(repo, this));
    mTable->setItemDelegate(new BranchDelegate(repo, mTable));

    // Set section resize mode after model is set.
    mTable->horizontalHeader()->setSectionResizeMode(
      BranchTableModel::Upstream, QHeaderView::Stretch);

    Footer *footer = new Footer(mTable);
    footer->setPlusEnabled(repo.head().isValid());

    connect(footer, &Footer::plusClicked, this, [this, repo] {
      NewBranchDialog *dialog = new NewBranchDialog(repo, git::Commit(), this);
      connect(dialog, &QDialog::accepted, this, [repo, dialog] {
        QString name = dialog->name();
        git::Commit commit = dialog->target();
        git::Branch branch = git::Repository(repo).createBranch(name, commit);

        // Start tracking.
        if (branch.isValid())
          branch.setUpstream(dialog->upstream());
      });

      dialog->open();
    });

    connect(footer, &Footer::minusClicked, this, [this] {
      // Get all selected branches before removing any.
      QList<git::Branch> branches;
      QModelIndexList indexes = mTable->selectionModel()->selectedRows();
      foreach (const QModelIndex &index, indexes) {
        QVariant var = index.data(BranchTableModel::BranchRole);
        branches.append(var.value<git::Branch>());
      }

      // Remove them all.
      foreach (git::Branch branch, branches) {
        Q_ASSERT(!branch.isHead());
        DeleteBranchDialog dialog(branch, this);
        dialog.exec();
      }
    });

    // Enable/disable minus button.
    auto updateMinusButton = [this, footer] {
      QModelIndexList indexes = mTable->selectionModel()->selectedRows();
      bool enabled = !indexes.isEmpty();
      foreach (const QModelIndex &index, indexes) {
        QVariant var = index.data(BranchTableModel::BranchRole);
        if (var.value<git::Branch>().isHead()) {
          enabled = false;
          break;
        }
      }

      footer->setMinusEnabled(enabled);
    };

    connect(mTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, updateMinusButton);
    connect(mTable->model(), &QAbstractItemModel::modelReset,
            this, updateMinusButton);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->addWidget(mTable);
    layout->addWidget(footer);
  }

  void editBranch(const QString &name)
  {
    QAbstractItemModel *model = mTable->model();
    for (int i = 0; i < model->rowCount(); ++i) {
      QModelIndex index = model->index(i, BranchTableModel::Name);
      if (index.data().toString() == name) {
        mTable->edit(index);
        return;
      }
    }
  }

private:
  QTableView *mTable;
};

class SubmodulesPanel : public QWidget
{
public:
  SubmodulesPanel(RepoView *view, QWidget *parent = nullptr)
    : QWidget(parent)
  {
    QTableView *table = new QTableView(this);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::SelectedClicked);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setShowGrid(false);

    table->setModel(new SubmoduleTableModel(view->repo(), this));
    table->setItemDelegate(new SubmoduleDelegate(table));

    // Set section resize mode after model is set.
    table->horizontalHeader()->setSectionResizeMode(
      SubmoduleTableModel::Name, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(
      SubmoduleTableModel::Url, QHeaderView::Stretch);

    connect(table, &QTableView::doubleClicked, this, [view](const QModelIndex &index) {
      QVariant var = index.data(SubmoduleTableModel::SubmoduleRole);
      view->openSubmodule(var.value<git::Submodule>());
    });

    Footer *footer = new Footer(table);
    footer->setPlusEnabled(false);
    footer->setMinusEnabled(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->addWidget(table);
    layout->addWidget(footer);
  }
};

class SearchPanel : public QWidget
{
  Q_OBJECT

public:
  SearchPanel(RepoView *view, QWidget *parent = nullptr)
    : QWidget(parent)
  {
    using Signal = void (QSpinBox::*)(int);
    auto signal = static_cast<Signal>(&QSpinBox::valueChanged);

    git::Config config = view->repo().appConfig();
    Q_ASSERT(config.isValid());

    // enable
    QCheckBox *enable = new QCheckBox(tr("Enable indexing"), this);
    enable->setChecked(config.value<bool>("index.enable", true));
    connect(enable, &QCheckBox::toggled, this, [view](bool checked) {
      git::Config config = view->repo().appConfig();
      config.setValue("index.enable", checked);

      if (checked) {
        view->startIndexing();
      } else {
        view->cancelIndexing();
      }
    });

    // commit term limit
    QSpinBox *terms = new QSpinBox(this);
    QLabel *termsLabel = new QLabel(tr("terms"), this);
    terms->setMinimum(100000);
    terms->setMaximum(99999999);
    terms->setSingleStep(100000);
    terms->setValue(config.value<int>("index.termlimit", 1000000));
    connect(terms, signal, this, [view](int value) {
      view->repo().appConfig().setValue("index.termlimit", value);
    });

    QHBoxLayout *termsLayout = new QHBoxLayout;
    termsLayout->addWidget(terms);
    termsLayout->addWidget(termsLabel);
    termsLayout->addStretch();

    // diff context lines
    QSpinBox *context = new QSpinBox(this);
    QLabel *contextLabel = new QLabel(tr("lines"), this);
    context->setValue(config.value<int>("index.contextlines", 3));
    connect(context, signal, this, [view](int value) {
      view->repo().appConfig().setValue("index.contextlines", value);
    });

    QHBoxLayout *contextLayout = new QHBoxLayout;
    contextLayout->addWidget(context);
    contextLayout->addWidget(contextLabel);
    contextLayout->addStretch();

    QFormLayout *form = new QFormLayout;
    form->setContentsMargins(16,2,16,0);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->addRow(tr("Limit commits to:"), termsLayout);
    form->addRow(tr("Diff context:"), contextLayout);

    // Collect a list of widgets to disable when indexing is disabled.
    QList<QWidget *> widgets = {
      terms, termsLabel, form->labelForField(termsLayout),
      context, contextLabel, form->labelForField(contextLayout)
    };

    auto setWidgetsEnabled = [widgets](bool enabled) {
      foreach (QWidget *widget, widgets)
        widget->setEnabled(enabled);
    };

    connect(enable, &QCheckBox::toggled, setWidgetsEnabled);
    setWidgetsEnabled(enable->isChecked());

    // remove
    QPushButton *remove = new QPushButton(tr("Remove Index"), this);
    remove->setEnabled(view->index()->isValid());
    connect(remove, &QPushButton::clicked, this, [view, remove] {
      Index *index = view->index();
      view->cancelIndexing();
      index->remove();
      remove->setEnabled(index->isValid());
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24,24,24,24);
    layout->addWidget(enable);
    layout->addLayout(form);
    layout->addWidget(remove, 0, Qt::AlignLeft);
    layout->addStretch();
  }
};

class LfsPanel : public QWidget
{
  Q_OBJECT

public:
  LfsPanel(RepoView *view, QWidget *parent = nullptr)
    : QWidget(parent)
  {
    if (!view->repo().lfsIsInitialized()) {
      QPushButton *button = new QPushButton(tr("Initialize LFS"), this);
      connect(button, &QPushButton::clicked, this, [this, view] {
        view->lfsInitialize();
        window()->close();
      });

      QVBoxLayout *layout = new QVBoxLayout(this);
      layout->addSpacing(16);
      layout->addWidget(button, 0, Qt::AlignCenter);
      layout->addSpacing(12);
      return;
    }

    git::Repository repo = view->repo();

    QListView *list = new QListView(this);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setSelectionBehavior(QAbstractItemView::SelectRows);
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QStringListModel *model = new QStringListModel(QStringList(), this);
    list->setModel(model);

    QFutureWatcher<QStringList> *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [model, watcher] {
      model->setStringList(watcher->result());
      watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run(repo, &git::Repository::lfsTracked));

    Footer *footer = new Footer(list);
    connect(footer, &Footer::plusClicked, this, [this, repo, model] {
      QDialog *dialog = new QDialog(this);
      dialog->setAttribute(Qt::WA_DeleteOnClose);

      QLabel *description = new QLabel(tr(
        "Specify a glob pattern for tracking large files.\n"
        "\n"
        "Generally, large files are greater than 500kB, change frequently,\n"
        "and do not compress well with git. This includes binary or video\n"
        "files which are already highly compressed.\n"
        "\n"
        "Examples\n"
        "*.png\n"
        "*.[pP][nN][gG]\n"
        "/images/*\n"
      ));

      QFormLayout *form = new QFormLayout;
      QLineEdit *pattern = new QLineEdit(dialog);
      form->addRow(tr("Pattern:"), pattern);

      QDialogButtonBox *buttons = new QDialogButtonBox();
      buttons->addButton(QDialogButtonBox::Cancel);
      QPushButton *track =
        buttons->addButton(tr("Track"), QDialogButtonBox::AcceptRole);
      track->setEnabled(false);
      connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
      connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

      QVBoxLayout *layout = new QVBoxLayout(dialog);
      layout->addWidget(description);
      layout->addLayout(form);
      layout->addWidget(buttons);

      connect(pattern, &QLineEdit::textChanged, this, [track](const QString &text) {
        track->setEnabled(!text.isEmpty());
      });

      connect(dialog, &QDialog::accepted, this, [pattern, repo, model] {
        git::Repository tmp(repo);
        tmp.lfsSetTracked(pattern->text(), true);
        model->setStringList(tmp.lfsTracked());
      });

      dialog->open();
    });

    connect(footer, &Footer::minusClicked, this, [list, repo, model] {
      git::Repository tmp(repo);
      QModelIndexList indexes = list->selectionModel()->selectedRows();
      foreach (const QModelIndex &index, indexes) {
        QString text = index.data(Qt::DisplayRole).toString();
        tmp.lfsSetTracked(text, false);
      }

      model->setStringList(tmp.lfsTracked());
    });

    // enable minus button
    auto updateMinusButton = [list, footer] {
      footer->setMinusEnabled(list->selectionModel()->hasSelection());
    };
    connect(list->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, updateMinusButton);
    connect(list->model(), &QAbstractItemModel::modelReset,
            this, updateMinusButton);

    QVBoxLayout *tableLayout = new QVBoxLayout;
    tableLayout->setSpacing(0);
    tableLayout->addWidget(list);
    tableLayout->addWidget(footer);

    QMap<QString,QString> map;
    foreach (const QString &string, repo.lfsEnvironment()) {
      if (string.contains("=")) {
        QString key = string.section('=', 0, 0);
        QString value = string.section('=', 1);
        map.insert(key, value);
      }
    }

    // url
    QLineEdit *urlLineEdit = new QLineEdit(
      map.value("Endpoint").section(" ", 0, 0));
    connect(urlLineEdit, &QLineEdit::textChanged,
    [repo](const QString &text) {
      git::Config config = repo.config();
      config.setValue("lfs.url", text);
    });

    // pruneoffsetdays
    QSpinBox *pruneOffsetDays = new QSpinBox(this);
    pruneOffsetDays->setValue(map.value("PruneOffsetDays").toInt());
    auto signal = QOverload<int>::of(&QSpinBox::valueChanged);
    connect(pruneOffsetDays, signal, [repo](int value) {
      git::Config config = repo.config();
      config.setValue("lfs.pruneoffsetdays", value);
    });
    QHBoxLayout *pruneOffsetLayout = new QHBoxLayout;
    pruneOffsetLayout->addWidget(pruneOffsetDays);
    pruneOffsetLayout->addWidget(new QLabel(tr("days")));
    pruneOffsetLayout->addStretch();

    // fetchrecentalways
    QCheckBox *fetchRecentAlways = new QCheckBox(
      tr("Fetch LFS objects from all references for the past"), this);
    bool fetchRecentEnabled = map.value("FetchRecentAlways").contains("true");
    fetchRecentAlways->setChecked(fetchRecentEnabled);
    connect(fetchRecentAlways, &QCheckBox::toggled,
    [repo](bool checked) {
      git::Config config = repo.config();
      config.setValue("lfs.fetchrecentalways", checked);
    });

    // fetchrecentrefsdays
    QSpinBox *fetchRecentRefsDays = new QSpinBox(this);
    fetchRecentRefsDays->setValue(map.value("FetchRecentRefsDays").toInt());
    fetchRecentRefsDays->setEnabled(fetchRecentEnabled);
    connect(fetchRecentRefsDays, signal, [repo](int value) {
      git::Config config = repo.config();
      config.setValue("lfs.fetchrecentrefsdays", value);
    });
    connect(fetchRecentAlways, &QCheckBox::toggled, this,
    [fetchRecentRefsDays](bool checked) {
      fetchRecentRefsDays->setEnabled(checked);
    });
    QHBoxLayout *refDaysLayout = new QHBoxLayout;
    refDaysLayout->addWidget(fetchRecentRefsDays);
    refDaysLayout->addWidget(new QLabel(tr("reference days or")));
    refDaysLayout->addStretch();

    // fetchrecentcommitsdays
    QSpinBox *fetchRecentCommitsDays = new QSpinBox(this);
    fetchRecentCommitsDays->setValue(map.value("FetchRecentCommitsDays").toInt());
    fetchRecentCommitsDays->setEnabled(fetchRecentEnabled);
    connect(fetchRecentCommitsDays, signal, [repo](int value) {
      git::Config config = repo.config();
      config.setValue("lfs.fetchrecentcommitsdays", value);
    });
    connect(fetchRecentAlways, &QCheckBox::toggled, this,
    [fetchRecentCommitsDays](bool checked) {
      fetchRecentCommitsDays->setEnabled(checked);
    });

    QHBoxLayout *commitDaysLayout = new QHBoxLayout;
    commitDaysLayout->addWidget(fetchRecentCommitsDays);
    commitDaysLayout->addWidget(new QLabel(tr("commit days")));
    commitDaysLayout->addStretch();

    // lfs environment
    QPushButton *environment = new QPushButton(tr("View Environment"));
    connect(environment, &QAbstractButton::clicked, this, [view] {
      git::Repository repo = view->repo();

      QDialog *dialog = new QDialog();
      dialog->setWindowTitle(tr("git-lfs env (read only)"));

      QSize size(500, 500);
      dialog->setFixedSize(size);

      QTextEdit *textEdit = new QTextEdit(dialog);
      textEdit->setFixedSize(size);
      textEdit->setReadOnly(true);

      foreach (const QString &string, repo.lfsEnvironment()) {
        textEdit->append(string);
      }

      dialog->exec();
    });

    QPushButton *deinit = new QPushButton(tr("Deinitialize LFS"));
    connect(deinit, &QAbstractButton::clicked, this, [this, view] {
      QString title = tr("Deinitialize LFS?");
      QString text =
        tr("Are you sure you want uninstall LFS from this repository?");

      QMessageBox msg(
        QMessageBox::Warning, title, text, QMessageBox::Cancel, this);

      QPushButton *agree =
      msg.addButton(tr("Deinitialize"), QMessageBox::AcceptRole);
      msg.exec();

      if (msg.clickedButton() == agree)
        view->lfsDeinitialize();

      window()->close();
    });

    QFormLayout *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->addRow(tr("Server URL:"), urlLineEdit);
    form->addRow(tr("Prune Offset:"), pruneOffsetLayout);
    form->addRow(tr("Fetch Recent:"), fetchRecentAlways);
    form->addRow(QString(), refDaysLayout);
    form->addRow(QString(), commitDaysLayout);
    form->addRow(tr("Advanced:"), environment);
    form->addRow(QString(), deinit);


    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(tableLayout);
    layout->addLayout(form);
  }
};

} // anon. namespace

ConfigDialog::ConfigDialog(RepoView *view, Index index)
  : QDialog(view)
{
  setMinimumWidth(500);
  setAttribute(Qt::WA_DeleteOnClose);
  setContextMenuPolicy(Qt::NoContextMenu);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);

  // Close on escape.
  QShortcut *esc = new QShortcut(tr("Esc"), this);
  connect(esc, &QShortcut::activated, this, &ConfigDialog::close);

  // Create tool bar.
  QToolBar *toolbar = new QToolBar(this);
  toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  toolbar->setMovable(false);
  layout->addWidget(toolbar);

  // Create central stack widget.
  mStack = new StackedWidget(this);
  connect(mStack, &StackedWidget::currentChanged,
          this, &ConfigDialog::adjustSize);

  layout->addWidget(mStack);

  // Track actions in a group.
  mActions = new QActionGroup(this);
  connect(mActions, &QActionGroup::triggered, this, [this](QAction *action) {
    mStack->setCurrentIndex(mActions->actions().indexOf(action));
    setWindowTitle(action->text());
  });

  // Add project panel.
  QAction *general = toolbar->addAction(QIcon(":/general.png"), tr("General"));
  general->setActionGroup(mActions);
  general->setCheckable(true);

  GeneralPanel *generalPanel = new GeneralPanel(view, this);
  mStack->addWidget(generalPanel);

  // Add diff panel.
  QAction *diff = toolbar->addAction(QIcon(":/diff.png"), tr("Diff"));
  diff->setActionGroup(mActions);
  diff->setCheckable(true);

  DiffPanel *diffPanel = new DiffPanel(view->repo(), this);
  mStack->addWidget(diffPanel);

  // Add remotes panel.
  QAction *remotes = toolbar->addAction(QIcon(":/remotes.png"), tr("Remotes"));
  remotes->setActionGroup(mActions);
  remotes->setCheckable(true);

  mStack->addWidget(new RemotesPanel(view->repo(), this));

  // Add branches panel.
  QAction *branches =
    toolbar->addAction(QIcon(":/branches.png"), tr("Branches"));
  branches->setActionGroup(mActions);
  branches->setCheckable(true);

  mStack->addWidget(new BranchesPanel(view->repo(), this));

  // Add submodules panel.
  QAction *submodules =
    toolbar->addAction(QIcon(":/submodules.png"), tr("Submodules"));
  submodules->setActionGroup(mActions);
  submodules->setCheckable(true);

  mStack->addWidget(new SubmodulesPanel(view, this));

  // Add search panel.
  QAction *search = toolbar->addAction(QIcon(":/search.png"), tr("Search"));
  search->setActionGroup(mActions);
  search->setCheckable(true);

  mStack->addWidget(new SearchPanel(view, this));

  // Add plugins panel.
  QAction *plugins = toolbar->addAction(QIcon(":/plugins.png"), tr("Plugins"));
  plugins->setActionGroup(mActions);
  plugins->setCheckable(true);

  mStack->addWidget(new PluginsPanel(view->repo(), this));

  // Add LFS panel.
  QAction *lfs = toolbar->addAction(QIcon(":/lfs.png"), tr("LFS"));
  lfs->setActionGroup(mActions);
  lfs->setCheckable(true);

  mStack->addWidget(new LfsPanel(view, this));

  // Trigger the requested action.
  mActions->actions().at(index)->trigger();

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  // Add edit button.
  QPushButton *edit =
    buttons->addButton(tr("Edit Config File..."), QDialogButtonBox::ResetRole);
  connect(edit, &QPushButton::clicked, this, [view, generalPanel] {
    QString file = view->repo().dir().filePath("config");
    if (EditorWindow *window = view->openEditor(file))
      connect(window->widget(), &BlameEditor::saved,
              generalPanel, &GeneralPanel::init);
  });

  // FIXME: Adding the button box directly to the layout
  // leaves weird margins at the sides of the dialog.
  QVBoxLayout *buttonLayout = new QVBoxLayout;
  buttonLayout->setContentsMargins(12,0,12,12);
  buttonLayout->addWidget(buttons);
  layout->addLayout(buttonLayout);
}

void ConfigDialog::addRemote(const QString &name)
{
  mActions->actions().at(Remotes)->trigger();
  static_cast<RemotesPanel *>(mStack->currentWidget())->addRemote(name);
}

void ConfigDialog::editBranch(const QString &name)
{
  mActions->actions().at(Branches)->trigger();
  static_cast<BranchesPanel *>(mStack->currentWidget())->editBranch(name);
}

void ConfigDialog::showEvent(QShowEvent *event)
{
  QDialog::showEvent(event);
  adjustSize();
}

#include "ConfigDialog.moc"
