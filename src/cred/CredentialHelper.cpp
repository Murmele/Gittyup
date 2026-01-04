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
#include "qtsupport.h"
#include "Store.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include <QLibrary>
#include <QPointer>
#include <QSettings>
#include <QTextStream>
#include <QTime>

namespace {

const QString kLogKey = "credential/log";

const QString cacheStoreName = "cache";
const QString storeStoreName = "git-credential-store";
const QString storeStoreNameOld = "store";
const QString osxKeyChainStoreName = "osxkeychain";
const QString winCredStoreName = "wincred";
const QString libSecretStoreName = "libsecret";
const QString gnomeKeyringStoreName = "gnome-keyring";

} // namespace

CredentialHelper *CredentialHelper::instance() {
  static QPointer<CredentialHelper> instance;
  if (!instance) {
    git::Config config = git::Config::global();
    auto helperName = config.value<QString>("credential.helper");
    if (isHelperValid(helperName)) {
      if (helperName == cacheStoreName) {
        instance = new Cache;
      } else if (helperName == storeStoreName ||
                 helperName == storeStoreNameOld) {
        auto path =
            QString::fromLocal8Bit(qgetenv("HOME") + "/.git-credentials");
        instance = new Store(path);
      } else {
        instance = new GitCredential(helperName);
      }
    }

    if (!instance)
      instance = new Cache;
  }

  return instance;
}

bool CredentialHelper::isHelperValid(const QString &name) {
  return !name.isEmpty();
}

QVector<CredentialHelper::HelperInformation>
CredentialHelper::getAvailableHelperInformation() {
  QVector<HelperInformation> list;
  list.append(HelperInformation(
      cacheStoreName, tr("Caching the credentials in the RAM. Required to "
                         "enter credentials on every startup")));
  list.append(HelperInformation(
      storeStoreName,
      tr("Storing the credentials encoded but unencrypted on disk. <a "
         "href=\"https://git-scm.com/docs/"
         "git-credential-store\">git-credential-store</a>")));
#if defined(Q_OS_MAC)
  list.append(
      HelperInformation(osxKeyChainStoreName, tr("MacOs credential manager")));
#elif defined(Q_OS_WIN)
  list.append(
      HelperInformation(winCredStoreName, tr("Windows credential manager")));
#else
  QLibrary lib("secret-1", 0);
  if (lib.load()) {
    list.append(HelperInformation(libSecretStoreName,
                                  tr("Secret Service D-Bus client library")));
  }
  // libsecret replaces libgnome-keyring.
  QLibrary lib2(gnomeKeyringStoreName, 0);
  if (lib2.load()) {
    list.append(HelperInformation(
        gnomeKeyringStoreName,
        tr("Prefer <a "
           "href=\"https://wiki.gnome.org/Projects/Libsecret\">libsecret</a> "
           "over gnome-keyring if available")));
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
  QTextStream(&file) << time << " - " << text << Qt::endl;
}
