//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDir>
#include <QString>
#include <QVariant>

#include "Setting.h"

class Settings : public QObject {
  Q_OBJECT

public:
  QVariant value(Setting::Id id) const;
  QVariant value(Setting::Id id, const QVariant &defaultValue) const;
  void setValue(Setting::Id id, const QVariant &value);

  // Look up lexer name by file name.
  QString lexer(const QString &filename);
  QString kind(const QString &filename);

  // prompt dialogs
  bool prompt(Prompt::Kind kind) const;
  void setPrompt(Prompt::Kind kind, bool prompt);
  QString promptDescription(Prompt::Kind kind) const;

  void setHotkey(const QString &action, const QString &hotkey);
  QString hotkey(const QString &action) const;

  // wrap lines in TextEditor in DiffView
  bool isTextEditorWrapLines() const;
  void setTextEditorWrapLines(bool wrapLines);

  // ignore whitespace
  bool isWhitespaceIgnored() const;
  void setWhitespaceIgnored(bool ignored);

  // last repository path
  QString lastPath() const;
  void setLastPath(const QString &lastPath);

  // settings directories
  static QDir rootDir();
  static QDir appDir();
  static QDir docDir();
  static QDir confDir();
  static QDir l10nDir();
  static QDir dictionariesDir();
  static QDir lexerDir();
  static QDir themesDir();
  static QDir pluginsDir();

  static QDir userDir();

  static QDir tempDir();

  static Settings *instance();

signals:
  void settingsChanged(bool refresh = false);

private:
  Settings(QObject *parent = nullptr);

  QString group() const;

  QVariant value(const QString &key) const;
  QVariant value(const QString &key, const QVariant &defaultValue) const;
  QVariant defaultValue(const QString &key) const;
  void setValue(const QString &key, const QVariant &value,
                bool refresh = false);

  QStringList mGroup;
  QVariantMap mDefaults;
  QVariantMap mCurrentMap;
};

#endif
