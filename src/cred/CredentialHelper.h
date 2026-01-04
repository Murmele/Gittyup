//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef CREDENTIALHELPER_H
#define CREDENTIALHELPER_H

#include <QObject>
#include <QString>

// This corresponds roughly to the git credential helper interface.
// It is used for both authenticating git remote connections and
// securely storing passwords associated with host accounts.
class CredentialHelper : public QObject {
public:
  // Username can be supplied by the caller to lookup a specific
  // account or filled in by the helper to get the first account
  // for the given host. Password will be filled in on success.
  virtual bool get(const QString &url, QString &username,
                   QString &password) = 0;

  virtual bool store(const QString &url, const QString &username,
                     const QString &password) = 0;

  // Get the correct helper for the current platform.
  static CredentialHelper *instance();

  static bool isLoggingEnabled();
  static void setLoggingEnabled(bool enabled);

  struct HelperInformation {
    HelperInformation(const QString &name, const QString &description)
        : name(name), description(description) {}
    QString name;
    QString description;
  };

  static QVector<HelperInformation> getAvailableHelperInformation();
  static bool isHelperValid(const QString &name);

protected:
  static void log(const QString &text);
};

#endif
