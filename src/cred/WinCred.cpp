//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "WinCred.h"
#include <QStringList>
#include <QUrl>
#include <windows.h>
#include <wincred.h>

// Attempt to be compatible with the git wincred helper by matching
// the target name and encoding the password in UTF-16.

namespace {

const QString kNameFmt = "%1@%2";
const QString kTargetFmt = "git:https://%1";

QString buildTarget(const QString &host, const QString &name) {
  return kTargetFmt.arg(name.isEmpty() ? host : kNameFmt.arg(name, host));
}

QString host(const QString &url) {
  QString host = QUrl(url).host();
  if (!host.isEmpty())
    return host;

  // Extract hostname from SSH URL.
  int end = url.indexOf(':');
  int begin = url.indexOf('@') + 1;
  return url.mid(begin, end - begin);
}

} // namespace

WinCred::WinCred() {}

bool WinCred::get(const QString &url, QString &username, QString &password) {
  log(QString("get: %1 %2").arg(url, username));

  PCREDENTIAL cred;
  QString target = buildTarget(host(url), username);
  if (!CredRead(target.toUtf8(), CRED_TYPE_GENERIC, 0, &cred)) {
    switch (DWORD error = GetLastError()) {
      case ERROR_NOT_FOUND:
        log(QString("get: credential not found for '%1'").arg(target));
        break;

      case ERROR_NO_SUCH_LOGON_SESSION:
        log("get: no such logon session");
        break;

      case ERROR_INVALID_FLAGS:
        log("get: invalid flags");
        break;

      default:
        log(QString("get: unknown error '%1'").arg(error));
        break;
    }

    return false;
  }

  username = cred->UserName;
  int size = cred->CredentialBlobSize / sizeof(ushort);
  password = QString::fromUtf16((ushort *)cred->CredentialBlob, size);

  CredFree(cred);

  return true;
}

bool WinCred::store(const QString &url, const QString &username,
                    const QString &password) {
  log(QString("store: %1 %2").arg(url, username));

  bool result = false;
  QByteArray name = username.toUtf8();
  QStringList names = {QString(), username};
  foreach (const QString &tmp, names) {
    QByteArray target = buildTarget(host(url), tmp).toUtf8();

    CREDENTIAL cred;
    cred.Flags = 0;
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = target.data();
    cred.Comment = "Written by GitAhead";
    cred.CredentialBlobSize = password.length() * sizeof(ushort);
    cred.CredentialBlob = (LPBYTE)password.utf16();
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.AttributeCount = 0;
    cred.Attributes = nullptr;
    cred.TargetAlias = nullptr;
    cred.UserName = name.data();
    if (CredWrite(&cred, 0)) {
      result = true;
    } else {
      switch (DWORD error = GetLastError()) {
        case ERROR_NO_SUCH_LOGON_SESSION:
          log("store: no such logon session");
          break;

        case ERROR_INVALID_PARAMETER:
          log("store: invalid parameter");

        case ERROR_INVALID_FLAGS:
          log("store: invalid flags");
          break;

        case ERROR_BAD_USERNAME:
          log(QString("store: bad username '%1'").arg(username));
          break;

        default:
          log(QString("store: unknown error '%1'").arg(error));
          break;
      }
    }
  }

  return result;
}
