//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include "Theme.h"
#include <QApplication>

class QNetworkAccessManager;
class QNetworkReply;
class QSslError;
class QUrlQuery;

class Application : public QApplication {
  Q_OBJECT

public:
  Application(int &argc, char **argv, bool haltOnParseError = false);

  void autoUpdate();
  bool restoreWindows();
  bool runSingleInstance();

  static bool isInTest();
  static void setInTest();

  static Theme *theme();

protected:
  bool event(QEvent *event) override;

private:
  void registerService();
  void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

  QString mPathspec = QString();
  QScopedPointer<Theme> mTheme;
  QStringList mPositionalArguments;

  static bool mIsInTest;
};

#ifdef Q_OS_LINUX
class DBusGittyup : public QObject {
  Q_OBJECT

public:
  DBusGittyup(QObject *parent = nullptr);

public slots:
  Q_SCRIPTABLE void openRepository(const QString &repo);
  Q_SCRIPTABLE void openAndFocusRepository(const QString &repo);
  Q_SCRIPTABLE void setFocus();
};
#endif

#endif
