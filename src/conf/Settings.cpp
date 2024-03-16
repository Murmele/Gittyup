//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "Settings.h"
#include "ConfFile.h"
#include "Debug.h"
#include "qtsupport.h"
#include "languages.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#define CS Qt::CaseInsensitive
#else
#define CS Qt::CaseSensitive
#endif

namespace {

const QString kWrapLines("diff/lines/wrap");
const QString kIgnoreWsKey("diff/whitespace/ignore");
const QString kLastPathKey("lastpath");
const QString kTranslation("translation");
const QString kTranslationLanguage("language");

// Look up variant at key relative to root.
QVariant lookup(const QVariantMap &root, const QString &key) {
  QStringList list(key.split("/", Qt::SkipEmptyParts));
  if (list.isEmpty())
    return root;

  QVariantMap map(root);
  while (map.contains(list.first())) {
    QVariant result(map.value(list.takeFirst()));
    if (list.isEmpty())
      return result;
    map = result.toMap();
  }

  return QVariant();
}

QString promptKey(Prompt::Kind kind) { return Prompt::key(kind); }
} // namespace

Settings::Settings(QObject *parent) : QObject(parent) {
  foreach (const QFileInfo &file, confDir().entryInfoList(QStringList("*.lua")))
    mDefaults[file.baseName()] = ConfFile(file.absoluteFilePath()).parse();
  mDefaults[kLastPathKey] = QDir::homePath();
  QVariantMap map;
  map[kTranslationLanguage] = QVariant(Languages::system);
  mDefaults[kTranslation] = map;
  mDefaults[kTranslation].toMap()[kTranslationLanguage] = Languages::system;
  mCurrentMap = mDefaults;
}

QString Settings::group() const { return mGroup.join("/"); }

QVariant Settings::value(const QString &key) const {
  return value(key, defaultValue(key));
}

QVariant Settings::value(const QString &key,
                         const QVariant &defaultValue) const {
  QSettings settings;
  settings.beginGroup(group());
  QVariant result(settings.value(key, defaultValue));
  settings.endGroup();
  return result;
}

QVariant Settings::defaultValue(const QString &key) const {
  return lookup(mCurrentMap, key);
}

void Settings::setValue(const QString &key, const QVariant &value,
                        bool refresh) {
  QSettings settings;
  settings.beginGroup(group());
  if (value == defaultValue(key)) {
    if (settings.contains(key)) {
      settings.remove(key);
      emit settingsChanged(refresh);
    }
  } else {
    if (value != settings.value(key)) {
      settings.setValue(key, value);
      emit settingsChanged(refresh);
    }
  }
  settings.endGroup();
}

QVariant Settings::value(Setting::Id id) const {
  return value(Setting::key(id));
}

QVariant Settings::value(Setting::Id id, const QVariant &defaultValue) const {
  return value(Setting::key(id), defaultValue);
}

void Settings::setValue(Setting::Id id, const QVariant &value) {
  setValue(Setting::key(id), value);
}

QString Settings::lexer(const QString &filename) {
  if (filename.isEmpty())
    return "null";

  QFileInfo info(filename);
  QString name(info.fileName());
  QString suffix(info.suffix().toLower());

  // Try all patterns first.
  QVariantMap lexers(mDefaults.value("lexers").toMap());
  foreach (const QString &key, lexers.keys()) {
    QVariantMap map(lexers.value(key).toMap());
    if (map.contains("patterns")) {
      foreach (QString pattern, map.value("patterns").toString().split(",")) {
        QRegExp regExp(pattern, CS, QRegExp::Wildcard);
        if (regExp.exactMatch(name))
          return key;
      }
    }
  }

  // Try to match by extension.
  foreach (const QString &key, lexers.keys()) {
    QVariantMap map(lexers.value(key).toMap());
    if (map.contains("extensions")) {
      foreach (QString ext, map.value("extensions").toString().split(",")) {
        if (suffix == ext)
          return key;
      }
    }
  }

  return "null";
}

QString Settings::kind(const QString &filename) {
  QString key(lexer(filename));
  QVariantMap lexers(mDefaults.value("lexers").toMap());
  return lexers.value(key).toMap().value("name").toString();
}

bool Settings::prompt(Prompt::Kind kind) const {
  return value(promptKey(kind)).toBool();
}

void Settings::setPrompt(Prompt::Kind kind, bool prompt) {
  setValue(promptKey(kind), prompt);
}

QString Settings::promptDescription(Prompt::Kind kind) const {
  switch (kind) {
    case Prompt::Kind::Stash:
      return tr("Prompt to edit stash message before stashing");

    case Prompt::Kind::Merge:
      return tr("Prompt to edit commit message before merging");

    case Prompt::Kind::Revert:
      return tr("Prompt to edit commit message before reverting");

    case Prompt::Kind::CherryPick:
      return tr("Prompt to edit commit message before cherry-picking");

    case Prompt::Kind::Directories:
      return tr("Prompt to stage directories");

    case Prompt::Kind::LargeFiles:
      return tr("Prompt to stage large files");
  }
  throw std::runtime_error("unreachable; value=" +
                           std::to_string(static_cast<int>(kind)));
}

void Settings::setHotkey(const QString &action, const QString &hotkey) {
  setValue("hotkeys/" + action, hotkey);
}

QString Settings::hotkey(const QString &action) const {
  return value("hotkeys/" + action, "").toString();
}

bool Settings::isTextEditorWrapLines() const {
  return value(kWrapLines).toBool();
}

void Settings::setTextEditorWrapLines(bool wrap) {
  setValue(kWrapLines, wrap, true);
}

bool Settings::isWhitespaceIgnored() const {
  return value(kIgnoreWsKey).toBool();
}

void Settings::setWhitespaceIgnored(bool ignored) {
  setValue(kIgnoreWsKey, ignored, true);
}

QString Settings::lastPath() const { return value(kLastPathKey).toString(); }

void Settings::setLastPath(const QString &lastPath) {
  setValue(kLastPathKey, lastPath);
}

QDir Settings::rootDir() {
  QDir dir(QCoreApplication::applicationDirPath());
  dir.cdUp();

  return dir;
}

QDir Settings::appDir() {
  QDir dir(QCoreApplication::applicationDirPath());

#ifdef Q_OS_MAC
  dir.cdUp(); // Contents
  dir.cdUp(); // bundle
  dir.cdUp(); // bundle dir
#endif

  return dir;
}

QDir Settings::docDir() { return confDir(); }

QDir Settings::confDir() {

#if !defined(NDEBUG)
  QDir dir(SRC_CONF_DIR);
#else
  QDir dir(rootDir());
  if (!dir.cd("Resources")) {
    if (!dir.cd(CONF_DIR))
      dir.setPath(SRC_CONF_DIR);
  }
#endif
  return dir;
}

QDir Settings::l10nDir() {
#if !defined(NDEBUG)
  QDir dir(QDir(SRC_L10N_DIR));
#else
  QDir dir(confDir());
  if (!dir.cd("l10n")) {
    dir = rootDir();
    if (!dir.cd(L10N_DIR))
      dir.setPath(SRC_L10N_DIR);
  }
#endif
  return dir;
}

QDir Settings::dictionariesDir() {
  QDir dir(confDir());
  dir.cd("dictionaries");
  return dir;
}

QDir Settings::lexerDir() {
#if !defined(NDEBUG)
  QDir dir(SRC_SCINTILLUA_LEXERS_DIR);
#else
  QDir dir(confDir());
  if (!dir.cd("lexers")) {
    dir = rootDir();
    if (!dir.cd(SCINTILLUA_LEXERS_DIR))
      dir.setPath(SRC_SCINTILLUA_LEXERS_DIR);
  }
#endif
  return dir;
}

QDir Settings::themesDir() {
  QDir dir(confDir());
  dir.cd("themes");
  return dir;
}

QDir Settings::pluginsDir() {
  QDir dir(confDir());
  dir.cd("plugins");
  return dir;
}

QDir Settings::userDir() {
  return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QDir Settings::tempDir() {
  QString name(QCoreApplication::applicationName());
  QDir dir(QDir::temp());
  dir.mkpath(name);
  dir.cd(name);
  return dir;
}

Settings *Settings::instance() {
  static Settings *instance = nullptr;
  if (!instance)
    instance = new Settings(qApp);

  return instance;
}
