//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "SettingsDialog.h"
#include "AboutDialog.h"
#include "DiffPanel.h"
#include "ExternalToolsDialog.h"
#include "HotkeysPanel.h"
#include "PluginsPanel.h"
#include "app/Application.h"
#include "app/CustomTheme.h"
#include "conf/Settings.h"
#include "cred/CredentialHelper.h"
#include "git/Config.h"
#include "log/LogEntry.h"
#include "tools/ExternalTool.h"
#include "ui/BlameEditor.h"
#include "ui/EditorWindow.h"
#include "ui/MainWindow.h"
#include "ui/MenuBar.h"
#include "ui/RepoView.h"
#include "update/Updater.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDirIterator>
#include <QFile>
#include <QFontComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QShortcut>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QToolBar>

#ifdef Q_OS_UNIX
#include "cli/Installer.h"
#endif

namespace {

void populateExternalTools(QComboBox *comboBox, const QString &type)
{
  comboBox->clear();

  QList<ExternalTool::Info> tools =
    ExternalTool::readGlobalTools(type) +
    ExternalTool::readBuiltInTools(type);

  QStringList names;
  foreach (const ExternalTool::Info &tool, tools) {
    if (tool.found)
      names.append(tool.name);
  }

  std::sort(names.begin(), names.end());
  foreach (const QString &tool, names)
    comboBox->addItem(tool);
}

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
  GeneralPanel(QWidget *parent = nullptr)
    : QWidget(parent)
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
    mNoTranslation = new QCheckBox(tr("No translation"), this);

    mStoreCredentials = new QCheckBox(
      tr("Store credentials in secure storage"), this);

    QLabel *privacy = new QLabel(tr("<a href='view'>View privacy policy</a>"));
    connect(privacy, &QLabel::linkActivated, [] {
      AboutDialog::openSharedInstance(AboutDialog::Privacy);
    });

    mSingleInstance = new QCheckBox(
      tr("Only allow a single running instance"), this);

    QFormLayout *form = new QFormLayout;
    form->addRow(tr("User name:"), mName);
    form->addRow(tr("User email:"), mEmail);
    form->addRow(tr("Automatic actions:"), fetchLayout);
    form->addRow(QString(), mPushCommit);
    form->addRow(QString(), mPullUpdate);
    form->addRow(QString(), mAutoPrune);
    form->addRow(tr("Language:"), mNoTranslation);
    form->addRow(tr("Credentials:"), mStoreCredentials);
    form->addRow(QString(), privacy);
    form->addRow(QString(), new QLabel(tr("%1 will be replaced with the repository's directory")));

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    form->addRow(tr("Single instance:"), mSingleInstance);
#endif

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16,12,16,12);
    layout->addLayout(form);

    init();

    // Connect signals after initializing fields.
    connect(mName, &QLineEdit::textChanged, [](const QString &text) {
      git::Config config = git::Config::global();
      config.setValue("user.name", text);
    });

    connect(mEmail, &QLineEdit::textChanged, [](const QString &text) {
      git::Config config = git::Config::global();
      config.setValue("user.email", text);
    });

    connect(mFetch, &QCheckBox::toggled, this, [](bool checked) {
      Settings::instance()->setValue("global/autofetch/enable", checked);
      foreach (MainWindow *window, MainWindow::windows()) {
        for (int i = 0; i < window->count(); ++i)
          window->view(i)->startFetchTimer();
      }
    });

    auto signal = QOverload<int>::of(&QSpinBox::valueChanged);
    connect(mFetchMinutes, signal, [](int value) {
      Settings::instance()->setValue("global/autofetch/minutes", value);
    });

    connect(mPushCommit, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("global/autopush/enable", checked);
    });

    connect(mPullUpdate, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("global/autoupdate/enable", checked);
    });

    connect(mAutoPrune, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("global/autoprune/enable", checked);
    });

    connect(mNoTranslation, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("translation/disable", checked);
    });

    connect(mStoreCredentials, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("credential/store", checked);
      delete CredentialHelper::instance();
    });

    connect(mSingleInstance, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("singleInstance", checked);
    });
  }

  void init()
  {
    git::Config config = git::Config::global();
    mName->setText(config.value<QString>("user.name"));
    mEmail->setText(config.value<QString>("user.email"));

    Settings *settings = Settings::instance();
    settings->beginGroup("global");
    settings->beginGroup("autofetch");
    mFetch->setChecked(settings->value("enable").toBool());
    mFetchMinutes->setValue(settings->value("minutes").toInt());
    settings->endGroup();

    mPushCommit->setChecked(settings->value("autopush/enable").toBool());
    mPullUpdate->setChecked(settings->value("autoupdate/enable").toBool());
    mAutoPrune->setChecked(settings->value("autoprune/enable").toBool());
    settings->endGroup();

    mNoTranslation->setChecked(settings->value("translation/disable").toBool());
    mStoreCredentials->setChecked(settings->value("credential/store").toBool());

    mSingleInstance->setChecked(settings->value("singleInstance").toBool());
  }

private:
  QLineEdit *mName;
  QLineEdit *mEmail;

  QCheckBox *mFetch;
  QSpinBox *mFetchMinutes;
  QCheckBox *mPushCommit;
  QCheckBox *mPullUpdate;
  QCheckBox *mAutoPrune;
  QCheckBox *mNoTranslation;
  QCheckBox *mStoreCredentials;
  QCheckBox *mSingleInstance;
};

class ToolsPanel : public QWidget
{
  Q_OBJECT

public:
  ToolsPanel(QWidget *parent = nullptr)
    : QWidget(parent), mConfig(git::Config::global())
  {
    // external editor
    QLineEdit *editTool = new QLineEdit(this);
    editTool->setText(mConfig.value<QString>("gui.editor"));
    connect(editTool, &QLineEdit::textChanged, this, [this](const QString &text) {
      if (text.isEmpty()) {
        mConfig.remove("gui.editor");
      } else {
        mConfig.setValue("gui.editor", text);
      }
    });

    // external diff/merge
    QHBoxLayout *diffTool = externalTools("diff");
    QHBoxLayout *mergeTool = externalTools("merge");

    // backup files
    QCheckBox *backup =
      new QCheckBox(tr("Keep backup of merge files (.orig)"), this);
    backup->setChecked(mConfig.value<bool>("mergetool.keepBackup"));
    connect(backup, &QCheckBox::toggled, this, [this](bool checked) {
      mConfig.setValue("mergetool.keepBackup", checked);
    });

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("External editor:"), editTool);
    layout->addRow(tr("External diff:"), diffTool);
    layout->addRow(tr("External merge:"), mergeTool);
    layout->addRow(tr("Backup files:"), backup);

    QLineEdit *mTerminalCommand = new QLineEdit(this);
    layout->addRow(tr("Terminal emulator command:"), mTerminalCommand);
    mTerminalCommand->setText(Settings::instance()->value("terminal/command").toString());

    connect(mTerminalCommand, &QLineEdit::textChanged, [](const QString &text) {
      Settings::instance()->setValue("terminal/command", text);
    });
    
    QLineEdit *mFileManagerCommand = new QLineEdit(this);
	QHBoxLayout* fileManagerLayout = new QHBoxLayout();
	fileManagerLayout->addWidget(mFileManagerCommand);
	fileManagerLayout->addWidget(new QLabel("\"%1\" = Repo Path", this));
	layout->addRow(tr("File manager command:"), fileManagerLayout);

    connect(mFileManagerCommand, &QLineEdit::textChanged, [](const QString &text) {
      Settings::instance()->setValue("filemanager/command", text);
    });
    mFileManagerCommand->setText(Settings::instance()->value("filemanager/command").toString());
  }

private:
  QHBoxLayout *externalTools(const QString &type)
  {
    // Fill combo box with git config entries.
    QComboBox *comboBox = new QComboBox(this);
    populateExternalTools(comboBox, type);

    // Read tool from git config.
    QString key = QString("%1.tool").arg(type);
    QString name = mConfig.value<QString>(key);
    comboBox->setCurrentIndex(comboBox->findText(name));

    // React to combo box selections.
    auto signal = QOverload<int>::of(&QComboBox::currentIndexChanged);
    connect(comboBox, signal, this, [this, key, comboBox](int index) {
      mConfig.setValue(key, comboBox->currentText());
    });

    QPushButton *configure = new QPushButton(tr("Configure"), this);
    connect(configure, &QPushButton::clicked, this, [this, comboBox, type] {
      ExternalToolsDialog *dialog = new ExternalToolsDialog(type, this);

      // Update combo box when external tools dialog closes.
      connect(dialog, &QDialog::finished, this, [comboBox, type] {
        QString name = comboBox->currentText();
        populateExternalTools(comboBox, type);
        comboBox->setCurrentIndex(comboBox->findText(name));
      });

      dialog->open();
    });

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(comboBox, 1);
    layout->addWidget(configure);

    return layout;
  }

  git::Config mConfig;
};

class WindowPanel : public QWidget
{
  Q_OBJECT

public:
  WindowPanel(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    Settings *settings = Settings::instance();
    settings->beginGroup("window");

    QComboBox *comboBox = new QComboBox(this);

    // default theme
    comboBox->addItem("Default");

    // predefined themes
    QDir dir = Settings::themesDir();
    dir.setNameFilters({"*.lua"});
    QDirIterator *it = new QDirIterator(dir);
    while (it->hasNext()) {
      it->next();
      QString name = it->fileInfo().baseName();
      if (name != "Default")
        comboBox->addItem(name);
    }

    // user themes
    bool exists = false;
    QDir appLocalDir = CustomTheme::userDir(false, &exists);
    if (exists) {
      appLocalDir.setNameFilters({"*.lua"});
      QDirIterator *it = new QDirIterator(appLocalDir);

      if (it->hasNext())
        comboBox->insertSeparator(comboBox->count());

      while (it->hasNext()) {
        it->next();
        comboBox->addItem(it->fileInfo().baseName(), it->filePath());
      }
    }

    comboBox->insertSeparator(comboBox->count());

    int index = comboBox->findText(settings->value("theme").toString());

    // add theme
    comboBox->addItem(tr("Add New Theme"));
    comboBox->addItem(tr("Edit Current Theme"), index);

    // Select the current theme.
    comboBox->setCurrentIndex(index >= 0 ? index : 0);

    // Edit enabled for user themes
    QStandardItemModel *model =
      static_cast<QStandardItemModel *>(comboBox->model());
    if(!comboBox->itemData(comboBox->currentIndex()).isValid())
      model->item(comboBox->count() - 1)->setEnabled(false);

    auto signal = QOverload<int>::of(&QComboBox::currentIndexChanged);
    connect(comboBox, signal, this, [this, parent, comboBox] {
      //Add new theme
      if (comboBox->currentIndex() == comboBox->count() - 2) {
        QDialog dialog;

        QDialogButtonBox *buttons =
          new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        QPushButton *create =
          buttons->addButton(tr("Create Theme"), QDialogButtonBox::AcceptRole);
        create->setEnabled(false);

        QLineEdit *nameField = new QLineEdit(&dialog);
        connect(nameField, &QLineEdit::textChanged, this, [create, nameField] {
          create->setEnabled(!nameField->text().isEmpty());
        });

        QFormLayout *layout = new QFormLayout(&dialog);
        layout->addRow(tr("Theme Name"), nameField);
        layout->addRow(buttons);

        if (dialog.exec()) {
          QDir dir = CustomTheme::userDir(true);
          QString path = dir.filePath(QString("%1.lua").arg(nameField->text()));
          QFile::copy(Settings::themesDir().filePath("Dark.lua"), path);
          EditorWindow::open(path);
        }

        window()->close();
        return;
      }

      //Edit current theme
      QStandardItemModel *model =
        static_cast<QStandardItemModel *>(comboBox->model());
      bool enabled = comboBox->itemData(comboBox->currentIndex()).isValid();
      model->item(comboBox->count() - 1)->setEnabled(enabled);

      if (comboBox->currentIndex() == comboBox->count() - 1) {
        int index = comboBox->currentData().toInt();
        QString path = comboBox->itemData(index).toString();
        EditorWindow::open(path);
        parent->close();
        return;
      }

      //Save theme
      Settings::instance()->setValue("window/theme", comboBox->currentText());

      QMessageBox mb(QMessageBox::Information, tr("Restart?"),
        tr("The application must be restarted for "
           "the theme change to take effect."));
      mb.setInformativeText(tr("Do you want to restart now?"));
      QPushButton *restart =
        mb.addButton(tr("Restart"), QMessageBox::AcceptRole);
      mb.addButton(tr("Later"), QMessageBox::RejectRole);
      mb.setDefaultButton(restart);
      mb.exec();

      if (mb.clickedButton() == restart) {
        QWidget *dialog = window();
        QTimer::singleShot(0, this, [dialog] {
          // Close the dialog.
          dialog->close();

          // Restart the app.
          QStringList args = qApp->arguments();
          QProcess::startDetached(args.takeFirst(), args);
          qApp->quit();
        });
      }
    });

    QCheckBox *fullPath = new QCheckBox(tr("Show full repository path"));
    fullPath->setChecked(settings->value("path/full").toBool());
    connect(fullPath, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("window/path/full", checked);
    });

    QCheckBox *hideLog = new QCheckBox(tr("Hide automatically"));
    hideLog->setChecked(settings->value("log/hide").toBool());
    connect(hideLog, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("window/log/hide", checked);
    });

    settings->beginGroup("tabs");
    QCheckBox *smTabs = new QCheckBox(tr("Open submodules in tabs"));
    smTabs->setChecked(settings->value("submodule").toBool());
    connect(smTabs, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("window/tabs/submodule", checked);
    });

    QCheckBox *repoTabs = new QCheckBox(tr("Open all repositories in tabs"));
    repoTabs->setChecked(settings->value("repository").toBool());
    connect(repoTabs, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("window/tabs/repository", checked);
    });
    settings->endGroup(); // tabs
    settings->endGroup(); // window

    QString mergeText = settings->promptDescription(Settings::PromptMerge);
    QCheckBox *merge = new QCheckBox(mergeText, this);
    merge->setChecked(settings->prompt(Settings::PromptMerge));
    connect(merge, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setPrompt(Settings::PromptMerge, checked);
    });

    QString revertText = settings->promptDescription(Settings::PromptRevert);
    QCheckBox *revert = new QCheckBox(revertText, this);
    revert->setChecked(settings->prompt(Settings::PromptRevert));
    connect(revert, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setPrompt(Settings::PromptRevert, checked);
    });

    QString cpText = settings->promptDescription(Settings::PromptCherryPick);
    QCheckBox *cherryPick = new QCheckBox(cpText, this);
    cherryPick->setChecked(settings->prompt(Settings::PromptCherryPick));
    connect(cherryPick, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setPrompt(Settings::PromptCherryPick, checked);
    });

    QString stashText = settings->promptDescription(Settings::PromptStash);
    QCheckBox *stash = new QCheckBox(stashText, this);
    stash->setChecked(settings->prompt(Settings::PromptStash));
    connect(stash, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setPrompt(Settings::PromptStash, checked);
    });

    QString largeFilesText = settings->promptDescription(Settings::PromptLargeFiles);
    QCheckBox *largeFiles = new QCheckBox(largeFilesText, this);
    largeFiles->setChecked(settings->prompt(Settings::PromptLargeFiles));
    connect(largeFiles, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setPrompt(Settings::PromptLargeFiles, checked);
    });

    QString directoriesText = settings->promptDescription(Settings::PromptDirectories);
    QCheckBox *directories = new QCheckBox(directoriesText, this);
    directories->setChecked(settings->prompt(Settings::PromptDirectories));
    connect(directories, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setPrompt(Settings::PromptDirectories, checked);
    });

    QFormLayout *layout = new QFormLayout(this);

    layout->addRow(tr("Theme:"), comboBox);
    layout->addRow(tr("Title:"), fullPath);
    layout->addRow(tr("Log:"), hideLog);
    layout->addRow(tr("Tabs:"), smTabs);
    layout->addRow(QString(), repoTabs);
    layout->addRow(tr("Prompt:"), merge);
    layout->addRow(QString(), revert);
    layout->addRow(QString(), cherryPick);
    layout->addRow(QString(), stash);
    layout->addRow(QString(), directories);
    layout->addRow(QString(), largeFiles);
  }
};

class EditorPanel : public QWidget
{
  Q_OBJECT

public:
  EditorPanel(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    auto spin = QOverload<int>::of(&QSpinBox::valueChanged);
    auto combo = QOverload<int>::of(&QComboBox::currentIndexChanged);

    Settings *settings = Settings::instance();
    settings->beginGroup("editor");

    settings->beginGroup("font");
    QFontComboBox *font = new QFontComboBox(this);
    font->setEditable(false);
    font->setFontFilters(QFontComboBox::MonospacedFonts);
    font->setCurrentText(settings->value("family").toString());
    connect(font, &QFontComboBox::currentTextChanged, [](const QString &text) {
      Settings::instance()->setValue("editor/font/family", text);
    });

    QSpinBox *fontSize = new QSpinBox(this);
    fontSize->setRange(2, 32);
    fontSize->setValue(settings->value("size").toInt());
    connect(fontSize, spin, [](int i) {
      Settings::instance()->setValue("editor/font/size", i);
    });
    settings->endGroup(); // font

    settings->beginGroup("indent");
    QComboBox *indent = new QComboBox(this);
    indent->addItem(tr("Tabs"));
    indent->addItem(tr("Spaces"));
    indent->setCurrentIndex(settings->value("tabs").toBool() ? 0 : 1);
    connect(indent, combo, [](int i) {
      Settings::instance()->setValue("editor/indent/tabs", i == 0);
    });

    QSpinBox *indentWidth = new QSpinBox(this);
    indentWidth->setRange(1, 32);
    indentWidth->setValue(settings->value("width").toInt());
    connect(indentWidth, spin, [](int i) {
      Settings::instance()->setValue("editor/indent/width", i);
    });

    QSpinBox *tabWidth = new QSpinBox(this);
    tabWidth->setRange(1, 32);
    tabWidth->setValue(settings->value("tabwidth").toInt());
    connect(tabWidth, spin, [](int i) {
      Settings::instance()->setValue("editor/indent/tabwidth", i);
    });
    settings->endGroup(); // indent

    QCheckBox *blameHeatMap = new QCheckBox(tr("Show heat map"), this);
    blameHeatMap->setChecked(settings->value("blame/heatmap").toBool());
    connect(blameHeatMap, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("editor/blame/heatmap", checked);
    });

    settings->endGroup(); // editor

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("Font:"), font);
    layout->addRow(tr("Font size:"), fontSize);
    layout->addRow(tr("Indent using:"), indent);
    layout->addRow(tr("Indent width:"), indentWidth);
    layout->addRow(tr("Tab width:"), tabWidth);
    layout->addRow(tr("Blame margin:"), blameHeatMap);
  }
};

class UpdatePanel : public QWidget
{
  Q_OBJECT

public:
  UpdatePanel(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    Settings *settings = Settings::instance();
    settings->beginGroup("update");

    QString checkText = tr("Check for updates automatically");
    QCheckBox *check = new QCheckBox(checkText, this);
    check->setChecked(settings->value("check").toBool());
    connect(check, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("update/check", checked);
    });

    QString downloadText = tr("Automatically download and install updates");
    QCheckBox *download = new QCheckBox(downloadText, this);
    download->setChecked(settings->value("download").toBool());
    connect(download, &QCheckBox::toggled, [](bool checked) {
      Settings::instance()->setValue("update/download", checked);
    });

    QPushButton *button = new QPushButton(tr("Check Now"), this);
    connect(button, &QPushButton::clicked,
            Updater::instance(), &Updater::update);

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("Software Update:"), check);
    layout->addRow(QString(), download);
    layout->addRow(QString(), button);

    settings->endGroup();
  }
};

class MiscPanel : public QWidget
{
  Q_OBJECT

public:
  MiscPanel(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    Settings *settings = Settings::instance();

    QLineEdit *sshConfigPathBox = new QLineEdit(settings->value("ssh/configFilePath").toString(), this);
    connect(sshConfigPathBox, &QLineEdit::textChanged, [](const QString &text) {
      Settings::instance()->setValue("ssh/configFilePath", text);
    });

    QLineEdit *sshKeyPathBox = new QLineEdit(settings->value("ssh/keyFilePath").toString(), this);
    connect(sshKeyPathBox, &QLineEdit::textChanged, [](const QString &text) {
      Settings::instance()->setValue("ssh/keyFilePath", text);
    });

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("Path to SSH config file:"), sshConfigPathBox);
    layout->addRow(tr("Path to default / fallback SSH key file:"), sshKeyPathBox);
  }
};

#ifdef Q_OS_UNIX
class TerminalPanel : public QWidget
{
  Q_OBJECT

public:
  TerminalPanel(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    Settings *settings = Settings::instance();
    settings->beginGroup("terminal");

    mNameBox = new QLineEdit(settings->value("name").toString(), this);
    connect(mNameBox, &QLineEdit::textChanged, [this](const QString &text) {
      Settings::instance()->setValue("terminal/name", text);
      updateInstallButton();
    });

    mPathBox = new QLineEdit(settings->value("path").toString(), this);
    connect(mPathBox, &QLineEdit::textChanged, [this](const QString &text) {
      Settings::instance()->setValue("terminal/path", text);
      updateInstallButton();
    });

    settings->endGroup();

    mInstallButton = new QPushButton(tr("Install"), this);
    connect(mInstallButton, &QPushButton::clicked, [this] {
      Installer installer(mNameBox->text(), mPathBox->text());
      if (installer.isInstalled()) {
        installer.uninstall();
      } else if (!installer.exists()) {
        installer.install();
      }

      updateInstallButton();
    });

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(tr("Name:"), mNameBox);
    layout->addRow(tr("Location:"), mPathBox);
    layout->addRow(QString(), mInstallButton);

    updateInstallButton();
  }

private:
  void updateInstallButton()
  {
    Installer installer(mNameBox->text(), mPathBox->text());
    bool installed = installer.isInstalled();
    mInstallButton->setText(!installed ? tr("Install") : tr("Uninstall"));
    mInstallButton->setEnabled(installed || !installer.exists());
  }

  QLineEdit *mNameBox;
  QLineEdit *mPathBox;
  QPushButton *mInstallButton;
};
#endif

} // anon. namespace

SettingsDialog::SettingsDialog(Index index, QWidget *parent)
  : QMainWindow(parent, Qt::Dialog)
{
  setAttribute(Qt::WA_DeleteOnClose);
  setUnifiedTitleAndToolBarOnMac(true);
  setContextMenuPolicy(Qt::NoContextMenu);

  // Close on escape.
  QShortcut *esc = new QShortcut(tr("Esc"), this);
  connect(esc, &QShortcut::activated, this, &SettingsDialog::close);

  // Create tool bar.
  QToolBar *toolbar = new QToolBar(this);
  toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  toolbar->setMovable(false);
  addToolBar(toolbar);

  // Create central stack widget.
  StackedWidget *stack = new StackedWidget(this);
  connect(stack, &StackedWidget::currentChanged,
          this, &SettingsDialog::adjustSize);

  QString text =
    tr("Global git settings can be overridden for each repository in "
       "the corresponding repository configuration page.");
  QLabel *description = new QLabel(text, this);
  description->setStyleSheet("QLabel { padding: 0px 20px 0px 20px }");
  description->setWordWrap(true);

#ifndef Q_OS_WIN
  QFont small = font();
  small.setPointSize(small.pointSize() - 2);
  description->setFont(small);
#endif

  QDialogButtonBox *buttons =
    new QDialogButtonBox(QDialogButtonBox::Ok, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::close);
  connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::close);

  // Add edit button.
  QPushButton *edit =
    buttons->addButton(tr("Edit Config File..."), QDialogButtonBox::ResetRole);

  QWidget *widget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(widget);
  layout->addWidget(stack);
  layout->addWidget(description);

#ifdef Q_OS_WIN
  layout->addSpacing(16);
#endif

  layout->addWidget(buttons);

  setCentralWidget(widget);

  // Track actions in a group.
  QActionGroup *actions = new QActionGroup(this);
  connect(actions, &QActionGroup::triggered, this,
  [this, stack, description, edit](QAction *action) {
    int index = action->data().toInt();
    bool config = (index < Window);
    description->setVisible(config);
    edit->setVisible(config);
    stack->setCurrentIndex(index);
    setWindowTitle(action->text());
  });

  // Add global git settings panel.
  QAction *general = toolbar->addAction(QIcon(":/general.png"), tr("General"));
  general->setData(General);
  general->setActionGroup(actions);
  general->setCheckable(true);

  stack->addWidget(new GeneralPanel(this));

  // Add diff panel.
  QAction *diff = toolbar->addAction(QIcon(":/diff.png"), tr("Diff"));
  diff->setData(Diff);
  diff->setActionGroup(actions);
  diff->setCheckable(true);

  stack->addWidget(new DiffPanel(git::Repository(), this));

  // Add tools panel.
  QAction *tools = toolbar->addAction(QIcon(":/tools.png"), tr("Tools"));
  tools->setData(Tools);
  tools->setActionGroup(actions);
  tools->setCheckable(true);

  stack->addWidget(new ToolsPanel(this));

  toolbar->addSeparator();

  // Add window panel.
  QAction *window = toolbar->addAction(QIcon(":/window.png"), tr("Window"));
  window->setData(Window);
  window->setActionGroup(actions);
  window->setCheckable(true);

  stack->addWidget(new WindowPanel(this));

  // Add editor panel.
  QAction *editor = toolbar->addAction(QIcon(":/editor.png"), tr("Editor"));
  editor->setData(Editor);
  editor->setActionGroup(actions);
  editor->setCheckable(true);

  stack->addWidget(new EditorPanel(this));

  // Add update panel.
  QAction *update = toolbar->addAction(QIcon(":/update.png"), tr("Update"));
  update->setData(Update);
  update->setActionGroup(actions);
  update->setCheckable(true);

  stack->addWidget(new UpdatePanel(this));

  // Add plugins panel.
  QAction *plugins = toolbar->addAction(QIcon(":/plugins.png"), tr("Plugins"));
  plugins->setData(Plugins);
  plugins->setActionGroup(actions);
  plugins->setCheckable(true);

  stack->addWidget(new PluginsPanel(git::Repository(), this));

  // Add misc panel.
  QAction *misc = toolbar->addAction(QIcon(":/misc.png"), tr("Misc"));
  misc->setData(Misc);
  misc->setActionGroup(actions);
  misc->setCheckable(true);

  stack->addWidget(new MiscPanel(this));

  // Add hotkeys panel.
  QAction *hotkeys = toolbar->addAction(QIcon(":/hotkeys.png"), tr("Hotkeys"));
  hotkeys->setData(Hotkeys);
  hotkeys->setActionGroup(actions);
  hotkeys->setCheckable(true);

  stack->addWidget(new HotkeysPanel(this));

#ifdef Q_OS_UNIX
  // Add terminal panel.
  QAction *terminal = toolbar->addAction(QIcon(":/terminal.png"), tr("Terminal"));
  terminal->setData(Terminal);
  terminal->setActionGroup(actions);
  terminal->setCheckable(true);

  stack->addWidget(new TerminalPanel(this));
#endif

  // Hook up edit button.
  connect(edit, &QPushButton::clicked, stack, [stack] {
    // Update on save.
    EditorWindow *window = EditorWindow::open(git::Config::globalPath());
    GeneralPanel *panel = static_cast<GeneralPanel *>(stack->widget(General));
    connect(window->widget(), &BlameEditor::saved, panel, &GeneralPanel::init);
  });

  // Select the requested index.
  actions->actions().at(index)->trigger();

  setMinimumWidth(toolbar->sizeHint().width());
}

void SettingsDialog::openSharedInstance(Index index)
{
  static QPointer<SettingsDialog> dialog;
  if (dialog) {
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
    return;
  }

  dialog = new SettingsDialog(index);
  dialog->show();
}

#include "SettingsDialog.moc"
