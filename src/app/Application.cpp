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
#include "update/Updater.h"
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
#include <QTimer>
#include <QTranslator>
#include <QUrlQuery>
#include <QUuid>

#if defined(Q_OS_MAC)
#include <unistd.h>

#elif defined(Q_OS_WIN)
#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>

static LPTOP_LEVEL_EXCEPTION_FILTER defaultFilter = nullptr;

static LONG WINAPI exceptionFilter(PEXCEPTION_POINTERS info)
{
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
  StringCchPrintf(fileName, MAX_PATH,
    "%s\\%s-%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
    dir, GITTYUP_NAME, GITTYUP_VERSION,
    localTime.wYear, localTime.wMonth, localTime.wDay,
    localTime.wHour, localTime.wMinute, localTime.wSecond,
    GetCurrentProcessId(), GetCurrentThreadId());

  HANDLE dumpFile = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

  MINIDUMP_EXCEPTION_INFORMATION expParam;
  expParam.ThreadId = GetCurrentThreadId();
  expParam.ExceptionPointers = info;
  expParam.ClientPointers = TRUE;

  MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
    dumpFile, MiniDumpWithDataSegs, &expParam, NULL, NULL);

  return defaultFilter ? defaultFilter(info) : EXCEPTION_CONTINUE_SEARCH;
}
#endif

namespace {

const QString kUserAgentFmt = "%1/%2 (%3)";

QString userAgentSystem()
{
#if defined(Q_OS_WIN)
  QOperatingSystemVersion current = QOperatingSystemVersion::current();
  if (current < QOperatingSystemVersion::Windows8) {
    return "Windows NT 6.1";
  } else if (current < QOperatingSystemVersion::Windows10) {
    return "Windows NT 6.2";
  } else {
    return "Windows NT 10.0";
  }
#elif defined(Q_OS_MAC)
  return "Macintosh";
#else
  return "Linux";
#endif
}

} // anon. namespace

Application::Application(int &argc, char **argv, bool haltOnParseError)
  : QApplication(argc, argv)
{
  Q_INIT_RESOURCE(resources);

  setApplicationName(GITTYUP_NAME);
  setApplicationVersion(GITTYUP_VERSION);
  setOrganizationDomain("gittyup.github.com");

  // Register types that are queued at runtime.
  qRegisterMetaType<git::Id>();

  // Connect updater signals.
  connect(Updater::instance(), &Updater::sslErrors,
          this, &Application::handleSslErrors);

  // Parse command line arguments.
  QCommandLineParser parser;
  parser.setApplicationDescription("Gittyup");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument(
    "repository", "Try to load a repository from the given path "
    "instead of the current working directory.", "[repository]");
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

  // Set pathspec filter.
  mPathspec = parser.value("filter");

  // Initialize theme.
  mTheme.reset(Theme::create(parser.value("theme")));
  setStyle(mTheme->style());
  setStyleSheet(mTheme->styleSheet());

  // Read translation settings
  QSettings settings;
  if ((!settings.value("translation/disable", false).toBool()) &&
      (!parser.isSet("no-translation"))) {
    // Load translation files.
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
  // Register service on Mac.
  registerService();

  // Load SF Mono font from Terminal.app.
  QDir dir("/System/Applications");
  if (!dir.exists())
    dir.setPath("/Applications");
  dir.cd("Utilities/Terminal.app/Contents/Resources/Fonts");
  foreach (const QString &name, dir.entryList({"SF*Mono-*.otf"}, QDir::Files))
    QFontDatabase::addApplicationFont(dir.filePath(name));

  // Create shared menu bar on Mac.
  (void) MenuBar::instance(nullptr);

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

  connect(this, &Application::aboutToQuit, [this] {
    // Clean up git library.
    // Make sure windows are really deleted.
    sendPostedEvents(nullptr, QEvent::DeferredDelete);
    git::Repository::shutdown();
  });

  // Read tracking settings.
  settings.beginGroup("tracking");
  QByteArray tid(GITTYUP_TRACKING_ID);
  if (!tid.isEmpty() && settings.value("enabled", true).toBool()) {
    // Get or create persistent client ID.
    mClientId = settings.value("id").toString();
    if (mClientId.isEmpty()) {
      mClientId = QUuid::createUuid().toString();
      settings.setValue("id", mClientId);
    }

    // Fire and forget, except to free the reply.
    mTrackingMgr = new QNetworkAccessManager(this);
    connect(mTrackingMgr, &QNetworkAccessManager::finished,
    [](QNetworkReply *reply) {
      reply->deleteLater();
    });
  }

  settings.endGroup();
}

void Application::autoUpdate()
{
  // Check for updates.
  if (Settings::instance()->value("update/check").toBool()) {
    // Check now.
    Updater::instance()->update(true);

    // Set a timer to check daily.
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, [] {
      Updater::instance()->update(true);
    });

    timer->start(24 * 60 * 60 * 1000);
  }
}

bool Application::restoreWindows()
{
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

Theme *Application::theme()
{
  return static_cast<Application *>(instance())->mTheme.data();
}

void Application::track(const QString &screen)
{
  QUrlQuery query;
  query.addQueryItem("t", "screenview");
  query.addQueryItem("cd", screen);

  static_cast<Application *>(instance())->track(query);
}

void Application::track(
  const QString &category,
  const QString &action,
  const QString &label,
  int value)
{
  QUrlQuery query;
  query.addQueryItem("t", "event");
  query.addQueryItem("ec", category);
  query.addQueryItem("ea", action);
  if (!label.isEmpty())
    query.addQueryItem("el", label);
  if (value >= 0)
    query.addQueryItem("ev", QString::number(value));

  static_cast<Application *>(instance())->track(query);
}

bool Application::event(QEvent *event)
{
  if (event->type() == QEvent::FileOpen)
    MainWindow::open(static_cast<QFileOpenEvent *>(event)->file());

  return QApplication::event(event);
}

void Application::track(const QUrlQuery &query)
{
  if (!mTrackingMgr)
    return;

  QString sys = userAgentSystem();
  QString language = QLocale().uiLanguages().first();
  QString userAgent = kUserAgentFmt.arg(GITTYUP_NAME, GITTYUP_VERSION, sys);

  QUrlQuery tmp = query;
  tmp.addQueryItem("v", "1");
  tmp.addQueryItem("ds", "app");
  tmp.addQueryItem("ul", language);
  tmp.addQueryItem("ua", userAgent);
  tmp.addQueryItem("an", GITTYUP_NAME);
  tmp.addQueryItem("av", GITTYUP_VERSION);
  tmp.addQueryItem("tid", GITTYUP_TRACKING_ID);
  tmp.addQueryItem("cid", mClientId);

//  QString header = "application/x-www-form-urlencoded";
//  QNetworkRequest request(QUrl("http://google-analytics.com/collect"));
//  request.setHeader(QNetworkRequest::ContentTypeHeader, header);

//  mTrackingMgr->post(request, tmp.query().toUtf8());
}

void Application::handleSslErrors(
  QNetworkReply *reply,
  const QList<QSslError> &errors)
{
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
