//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CredentialHelper.h"
#include "Cache.h"
#include "GitCredential.h"
#include "WinCred.h"
#include "conf/Settings.h"
#include <QLibrary>
#include <QPointer>
#include <QSettings>
#include <QTextStream>
#include <QTime>

namespace {

const QString kLogKey = "credential/log";

} // namespace

CredentialHelper *CredentialHelper::instance() {
  static QPointer<CredentialHelper> instance;
  if (!instance) {
    if (Settings::instance()->value(Setting::Id::StoreCredentials).toBool()) {
#if defined(Q_OS_MAC)
      instance = new GitCredential("osxkeychain");
#elif defined(Q_OS_WIN)
      // The git wincred helper fails for some users.
      instance = new WinCred;
#else
      QLibrary lib("secret-1", 0);
      if (lib.load()) {
        instance = new GitCredential("libsecret");
      } else {
        QLibrary lib("gnome-keyring", 0);
        if (lib.load())
          instance = new GitCredential("gnome-keyring");
      }
#endif
    }

    if (!instance)
      instance = new Cache;
  }

  return instance;
}

bool CredentialHelper::isLoggingEnabled() {
  return QSettings().value(kLogKey).toBool();
}

void CredentialHelper::setLoggingEnabled(bool enable) {
  QSettings().setValue(kLogKey, enable);
}

void CredentialHelper::log(const QString &text) {
  if (!isLoggingEnabled())
    return;

  QFile file(Settings::tempDir().filePath("cred.log"));
  if (!file.open(QFile::WriteOnly | QIODevice::Append))
    return;

  QString time = QTime::currentTime().toString(Qt::ISODateWithMs);
  QTextStream(&file) << time << " - " << text << endl;
}
