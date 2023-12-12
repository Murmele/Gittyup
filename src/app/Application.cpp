//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Application.h"
#include "conf/Settings.h"
#include "git/Id.h"
#include "git/Repository.h"
#include "ui/MainWindow.h"
#include "ui/MenuBar.h"
#include "ui/RepoView.h"
#include "ui/TabWidget.h"
#include "update/Updater.h"
#include "languages.h"
#include "util/Debug.h"
#include <QCloseEvent>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QOperatingSystemVersion>
#include <QSettings>
#include <QStyle>
#include <QTimer>
#include <QTranslator>
#include <QUrlQuery>
#include <QUuid>

bool Application::mIsInTest = false;

#if defined(Q_OS_LINUX)
#include <QtDBus/QtDBus>

#elif defined(Q_OS_MAC)
#include <unistd.h>

#elif defined(Q_OS_WIN)
#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <QWindow>

static LPTOP_LEVEL_EXCEPTION_FILTER defaultFilter = nullptr;

static LONG WINAPI exceptionFilter(PEXCEPTION_POINTERS info) {
  // Protect against reentering.
  static bool entered = false;
  if (entered)
    ExitProcess(1);
  entered = true;

  // Write dump file.
  SYSTEMTIME localTime;
  GetLocalTime(&localTime);

  char temp[MAX_PATH];
  GetTempPath(MAX_PATH, temp);

  char dir[MAX_PATH];
  StringCchPrintf(dir, MAX_PATH, "%sGittyup", temp);
  CreateDirectory(dir, NULL);

  char fileName[MAX_PATH];
  StringCchPrintf(
      fileName, MAX_PATH, "%s\\%s-%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
      dir, GITTYUP_NAME, GITTYUP_VERSION, localTime.wYear, localTime.wMonth,
      localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond,
      GetCurrentProcessId(), GetCurrentThreadId());

  HANDLE dumpFile =
      CreateFile(fileName, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  MINIDUMP_EXCEPTION_INFORMATION expParam;
  expParam.ThreadId = GetCurrentThreadId();
  expParam.ExceptionPointers = info;
  expParam.ClientPointers = TRUE;

  MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile,
                    MiniDumpWithDataSegs, &expParam, NULL, NULL);

  return defaultFilter ? defaultFilter(info) : EXCEPTION_CONTINUE_SEARCH;
}
#endif

Application::Application(int &argc, char **argv, bool haltOnParseError)
    : QApplication(argc, argv) {
  Q_INIT_RESOURCE(resources);

  setApplicationName(GITTYUP_NAME);
  setApplicationVersion(GITTYUP_VERSION);
  setOrganizationDomain("gittyup.github.com");

  // Register types that are queued at runtime.
  qRegisterMetaType<git::Id>();

  // Connect updater signals.
  connect(Updater::instance(), &Updater::sslErrors, this,
          &Application::handleSslErrors);

  // Parse command line arguments.
  QCommandLineParser parser;
  parser.setApplicationDescription("Gittyup" BUILD_DESCRIPTION);
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("repository",
                               "Try to load a repository from the given path "
                               "instead of the current working directory.",
                               "[repository]");
  parser.addOption({{"d", "debug-menu"}, "Show debug menu."});
  parser.addOption({{"t", "theme"}, "Choose theme.", "name"});
  parser.addOption({{"f", "filter"}, "Set the pathspec filter.", "pathspec"});
  parser.addOption({"no-translation", "Disable translation."});

  if (haltOnParseError) {
    parser.process(arguments());
  } else {
    parser.parse(arguments());
  }

  // Remember positional args.
  mPositionalArguments = parser.positionalArguments();

  // Set debug menu option.
  MenuBar::setDebugMenuVisible(parser.isSet("debug-menu"));

  Debug(QString("Root dir: %1").arg(Settings::rootDir().absolutePath()));
  Debug(QString("App dir: %1").arg(Settings::appDir().absolutePath()));
  Debug(QString("Doc dir: %1").arg(Settings::docDir().absolutePath()));
  Debug(QString("Conf dir: %1").arg(Settings::confDir().absolutePath()));
  Debug(QString("l10n dir: %1").arg(Settings::l10nDir().absolutePath()));
  Debug(QString("dictionaries dir: %1")
            .arg(Settings::dictionariesDir().absolutePath()));
  Debug(QString("lexer dir: %1").arg(Settings::lexerDir().absolutePath()));
  Debug(QString("themes dir: %1").arg(Settings::themesDir().absolutePath()));
  Debug(
      QString("pluginsDir dir: %1").arg(Settings::pluginsDir().absolutePath()));

  // Set pathspec filter.
  mPathspec = parser.value("filter");

#if defined(Q_OS_WIN)
  // Load default palette.
  QApplication::setPalette(QApplication::style()->standardPalette());
#endif

  // Initialize theme.
  mTheme.reset(Theme::create(parser.value("theme")));
  setStyle(mTheme->style());
  setStyleSheet(mTheme->styleSheet());

#if defined(Q_OS_WIN)
  // Set default font style and hinting.
  QFont font = QApplication::font();
  font.setStyleStrategy(QFont::PreferDefault);
  font.setHintingPreference(QFont::PreferDefaultHinting);
  QApplication::setFont(font);
#endif

  // Read translation settings.
  QSettings settings;
  if ((!settings.value(Setting::key(Setting::Id::DontTranslate), false)
            .toBool()) &&
      (!parser.isSet("no-translation"))) {
    // Load translation files.

    const auto &language =
        Settings::instance()->value(Setting::Id::Language).toString();
    if (language != Languages::system)
      QLocale::setDefault(QLocale(language));

    QLocale locale;
    QDir l10n = Settings::l10nDir();
    QString name = QString(GITTYUP_NAME).toLower();
    QTranslator *translator = new QTranslator(this);
    if (translator->load(locale, name, "_", l10n.absolutePath())) {
      installTranslator(translator);
    } else {
      delete translator;
    }

    // Load Qt translation file.
    QTranslator *qt = new QTranslator(this);
    if (qt->load(locale, "qtbase", "_", l10n.absolutePath())) {
      installTranslator(qt);
    } else {
      QDir dir(QT_TRANSLATIONS_DIR);
      if (dir.exists() && qt->load(locale, "qtbase", "_", dir.absolutePath())) {
        installTranslator(qt);
      } else {
        delete qt;
      }
    }
  }

  // Enable system proxy auto-detection.
  QNetworkProxyFactory::setUseSystemConfiguration(true);

  // Do platform-specific initialization.
#if defined(Q_OS_MAC)
  // Register service on macOS.
  registerService();

  // Load SF Mono font from Terminal.app.
  QDir dir("/System/Applications");
  if (!dir.exists())
    dir.setPath("/Applications");
  dir.cd("Utilities/Terminal.app/Contents/Resources/Fonts");
  foreach (const QString &name, dir.entryList({"SF*Mono-*.otf"}, QDir::Files))
    QFontDatabase::addApplicationFont(dir.filePath(name));

  // Create shared menu bar on macOS.
  (void)MenuBar::instance(nullptr);

#elif defined(Q_OS_WIN)
  // FIXME: Install in main before anything else?
  defaultFilter = SetUnhandledExceptionFilter(&exceptionFilter);

#elif defined(Q_OS_LINUX)
  QIcon icon;
  icon.addPixmap(QPixmap(":/Gittyup.iconset/icon_16x16.png"));
  icon.addPixmap(QPixmap(":/Gittyup.iconset/icon_32x32.png"));
  icon.addPixmap(QPixmap(":/Gittyup.iconset/icon_64x64.png"));
  icon.addPixmap(QPixmap(":/Gittyup.iconset/icon_128x128.png"));
  setWindowIcon(icon);
#endif

  // Set path to emoji description file.
  git::Commit::setEmojiFile(Settings::confDir().filePath("emoji.json"));

  // Initialize git library.
  git::Repository::init();

  connect(this, &Application::aboutToQuit, [] {
    // Clean up git library.
    // Make sure windows are really deleted.
    sendPostedEvents(nullptr, QEvent::DeferredDelete);
    git::Repository::shutdown();
  });
}

void Application::autoUpdate() {
  // Check for updates.
  if (Settings::instance()
          ->value(Setting::Id::CheckForUpdatesAutomatically)
          .toBool()) {
    // Check now.
    Updater::instance()->update(true);

    // Set a timer to check daily.
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout,
                     [] { Updater::instance()->update(true); });

    timer->start(24 * 60 * 60 * 1000);
  }
}

bool Application::restoreWindows() {
#ifdef Q_OS_MAC
  // Check for connection to a terminal.
  if (!isatty(fileno(stdin)))
    QDir::setCurrent(Settings::appDir().path());
#endif

  QDir dir = QDir::current();
  if (!mPositionalArguments.isEmpty()) {
    // Check for command line repo.
    QString arg = mPositionalArguments.first();
    if (QFileInfo(arg).isAbsolute()) {
      dir.setPath(arg);
    } else {
      dir.cd(arg);
    }
  }

  // Distinguish two different app modes. The normal mode is entered when the
  // current directory is the same as the app directory (e.g. because the user
  // launched the app by double-clicking in the GUI). It restores the session
  // from the last normal mode invocation. Command line mode tries to load a
  // repository from the current directory. Windows opened in this mode aren't
  // saved to settings. FIXME: Save subsequently opened windows?

  // Load command line arg.
  if (dir != Settings::appDir()) {
    if (MainWindow *win = MainWindow::open(dir.path(), false)) {
      win->currentView()->setPathspec(mPathspec);
      return true;
    }

    return false;
  }

  // Save on exit.
  MainWindow::setSaveWindowSettings(true);

  // Restore previous session.
  return MainWindow::restoreWindows();
}

static MainWindow *openOrSwitch(QDir repo) {
  repo.makeAbsolute();

  QList<MainWindow *> windows = MainWindow::windows();
  for (MainWindow *window : windows) {
    TabWidget *tabs = window->tabWidget();

    for (int i = 0; i < tabs->count(); ++i) {
      RepoView *view = (RepoView *)tabs->widget(i);
      QDir openRepo = QDir(view->repo().workdir().path());

      if (openRepo == repo) {
        tabs->setCurrentIndex(i);
        return window;
      }
    }
  }

  return MainWindow::open(repo.path(), true);
}

#if defined(Q_OS_LINUX)
#define DBUS_SERVICE_NAME GITTYUP_IDENTIFIER
#define DBUS_INTERFACE_NAME GITTYUP_DBUS_INTERFACE_NAME
#define DBUS_OBJECT_PATH GITTYUP_DBUS_OBJECT_PATH

DBusGittyup::DBusGittyup(QObject *parent) : QObject(parent) {}

void DBusGittyup::openRepository(const QString &repo) {
  openOrSwitch(QDir(repo));
}

void DBusGittyup::openAndFocusRepository(const QString &repo) {
  openOrSwitch(QDir(repo))->activateWindow();
}

void DBusGittyup::setFocus() { MainWindow::activeWindow()->activateWindow(); }

#elif defined(Q_OS_WIN)
#define COPYDATA_WINDOW_TITLE                                                  \
  "Gittyup WM_COPYDATA receiver 16b8b3f6-6446-4fa7-8c72-53c25b1f206c"
enum CopyDataCommand { Focus = 0, FocusAndOpen = 1 };

namespace {
// Helper window class for receiving IPC messages
class CopyDataWindow : public QWindow {
public:
  CopyDataWindow() { setTitle(COPYDATA_WINDOW_TITLE); }

protected:
  virtual bool nativeEvent(const QByteArray &eventType, void *message,
                           long *result) Q_DECL_OVERRIDE {
    MSG *msg = (MSG *)message;

    if (msg->message == WM_COPYDATA) {
      COPYDATASTRUCT *cds = (COPYDATASTRUCT *)msg->lParam;

      switch (cds->dwData) {
        case CopyDataCommand::Focus:
          MainWindow::activeWindow()->activateWindow();
          break;

        case CopyDataCommand::FocusAndOpen:
          if (cds->cbData % 2 == 0) {
            QString repo = QString::fromUtf16((const char16_t *)cds->lpData,
                                              cds->cbData / 2);
            openOrSwitch(QDir(repo));

            MainWindow::activeWindow()->activateWindow();
          }
          break;
      }

      return true;
    }

    return QWindow::nativeEvent(eventType, message, result);
  }
};
} // namespace
#endif

bool Application::runSingleInstance() {
  if (Settings::instance()
          ->value(Setting::Id::AllowSingleInstanceOnly)
          .toBool()) {
#if defined(Q_OS_LINUX)
    QDBusConnection bus = QDBusConnection::sessionBus();

    if (bus.isConnected()) {
      QDBusInterface masterInstance(DBUS_SERVICE_NAME, DBUS_OBJECT_PATH,
                                    DBUS_INTERFACE_NAME, bus);

      // Is another instance running on the current DBus session bus?
      if (masterInstance.isValid()) {
        if (!mPositionalArguments.isEmpty())
          masterInstance.call(
              "openAndFocusRepository",
              QDir(mPositionalArguments.first()).absolutePath());
        return true;
      }
    }

#elif defined(Q_OS_WIN)
    HWND handle = FindWindowA(nullptr, COPYDATA_WINDOW_TITLE);
    // Is another instance running in the current session?
    if (handle != nullptr) {
      QWindow sender;

      COPYDATASTRUCT cds;

      if (mPositionalArguments.isEmpty()) {
        cds.dwData = CopyDataCommand::Focus;
        cds.cbData = 0;
        cds.lpData = nullptr;

        SendMessage(handle, WM_COPYDATA, sender.winId(), (LPARAM)&cds);

      } else {
        QString arg = QDir(mPositionalArguments.first()).absolutePath();

        cds.dwData = CopyDataCommand::FocusAndOpen;
        cds.cbData = arg.length() * 2;
        cds.lpData = (LPVOID)arg.utf16();

        SendMessage(handle, WM_COPYDATA, sender.winId(), (LPARAM)&cds);
      }

      return true;
    }

#endif
  }

#ifndef Q_OS_MAC
  registerService();
#endif
  return false;
}

#ifndef Q_OS_MAC
void Application::registerService() {
#if defined(Q_OS_LINUX)
  QDBusConnection bus = QDBusConnection::sessionBus();

  if (!bus.isConnected())
    return;

  if (!bus.registerService(DBUS_SERVICE_NAME))
    return;

  bus.registerObject(DBUS_OBJECT_PATH, DBUS_INTERFACE_NAME, new DBusGittyup(),
                     QDBusConnection::ExportScriptableSlots);

#elif defined(Q_OS_WIN)
  CopyDataWindow *receiver = new CopyDataWindow();
  receiver->winId();
#endif
}
#endif

bool Application::isInTest() { return mIsInTest; }

void Application::setInTest() { mIsInTest = true; }

Theme *Application::theme() {
  return static_cast<Application *>(instance())->mTheme.data();
}

bool Application::event(QEvent *event) {
  if (event->type() == QEvent::FileOpen)
    MainWindow::open(static_cast<QFileOpenEvent *>(event)->file());

  return QApplication::event(event);
}

void Application::handleSslErrors(QNetworkReply *reply,
                                  const QList<QSslError> &errors) {
  QSettings settings;
  if (settings.value("ssl/ignore").toBool()) {
    // FIXME: Ignore individual errors?
    reply->ignoreSslErrors(errors);
    return;
  }

  QString title = tr("SSL Errors");
  QString text =
      tr("Failed to set up SSL session. Do you want to ignore these errors?");
  auto buttons = QMessageBox::Abort | QMessageBox::Ignore;
  QMessageBox msg(QMessageBox::Warning, title, text, buttons);

  QString message;
  foreach (const QSslError &error, errors)
    message.append(QString("<p>%1</p>").arg(error.errorString()));
  msg.setInformativeText(message);

  if (msg.exec()) {
    reply->ignoreSslErrors(errors);
    settings.setValue("ssl/ignore", true);
  }
}
