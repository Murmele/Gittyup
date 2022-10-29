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
#include "Store.h"
#include "conf/Settings.h"
#include <QLibrary>
#include <QPointer>
#include <QSettings>
#include <QTextStream>
#include <QTime>

namespace {

const QString kLogKey = "credential/log";

const QString cacheStoreName = "cache";
const QString storeStoreName = "store";
const QString osxKeyChainStoreName = "osxkeychain";
const QString winCredStoreName = "winCred";
const QString libSecretStoreName = "libsecret";
const QString gnomeKeyringStoreName = "gnome-keyring";

} // namespace

CredentialHelper *CredentialHelper::instance() {
  static QPointer<CredentialHelper> instance;
  if (!instance) {
    auto helperName = Settings::instance()->value(kStoreKey).toString();
    if (isHelperValid(helperName)) {
      if (helperName == cacheStoreName) {
        instance = new Cache;
      } else if (helperName == storeStoreName) {
        auto path =
            QString::fromLocal8Bit(qgetenv("HOME") + "/.git-credentials");
        instance = new Store(path);
      }
#if defined(Q_OS_WIN)
      else if (helperName == winCredStoreName) {
        instance = new WinCred;
      }
#endif
      else {
        instance = new GitCredential(helperName);
      }
    }

    if (!instance)
      instance = new Cache;
  }

  return instance;
}

bool CredentialHelper::isHelperValid(const QString &name) {
  auto helpers = getAvailableHelperNames();
  foreach (auto helper, helpers) {
    if (helper == name) {
      return true;
    }
  }
  return false;
}

QStringList CredentialHelper::getAvailableHelperNames() {
  QStringList list;
  list.append(cacheStoreName);
  list.append(storeStoreName);
#if defined(Q_OS_MAC)
  list.append(osxKeyChainStoreName);
#elif defined(Q_OS_WIN)
  list.append(winCredStoreName),
#else
  QLibrary lib("secret-1", 0);
  if (lib.load()) {
    list.append(libSecretStoreName);
  }
  QLibrary lib2(gnomeKeyringStoreName, 0);
  if (lib2.load()) {
    list.append(gnomeKeyringStoreName);
  }
#endif
  return list;
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
