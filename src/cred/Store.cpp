//
//          Copyright (c) 2022, Gittyup Community
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Hessamoddin Hediehloo(H-4ND-H)
//

#include "Store.h"
#include <QUrl>
#include <QFile>

namespace {

QString host(const QString &url) {
  QString host = QUrl(url).host();
  if (!host.isEmpty())
    return host;

  // Extract hostname from SSH URL.
  int end = url.indexOf(':');
  int begin = url.indexOf('@') + 1;
  return url.mid(begin, end - begin);
}

QString protocol(const QString &url) {
  QString scheme = QUrl(url).scheme();
  return !scheme.isEmpty() ? scheme : "ssh";
}

} // namespace

Store::Store(const QString &path) { mPath = path; }

QMap<QString, QMap<QString, QMap<QString, QString>>> Store::readCredFile() {
  QMap<QString, QMap<QString, QMap<QString, QString>>> store;

  QFile file(mPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return store;

  while (!file.atEnd()) {
    auto line = file.readLine();
    auto urlStr = QUrl::fromPercentEncoding(line);
    auto urlObj = QUrl::fromUserInput(urlStr);
    store[urlObj.scheme()][urlObj.host()][urlObj.userName()] =
        urlObj.password();
  }

  file.close();

  return store;
}

bool Store::extractUserPass(const QMap<QString, QString> &map,
                            QString &username, QString &password) {
  if (map.isEmpty())
    return false;

  if (username.isEmpty())
    username = map.keys().first();

  if (!map.contains(username))
    return false;

  password = map.value(username);

  return !username.isEmpty() && !password.isEmpty();
}

bool Store::get(const QString &url, QString &username, QString &password) {

  auto store = readCredFile();
  const QMap<QString, QString> &map = store[protocol(url)][host(url)];
  return extractUserPass(map, username, password);
}

bool Store::store(const QString &url, const QString &username,
                  const QString &password) {
  QUrl temp(url);
  temp.setScheme(protocol(url));
  temp.setHost(host(url));
  temp.setUserName(username);
  temp.setPassword(password);

  auto encoded = QUrl::toPercentEncoding(temp.toString(), "@:/");

  QFile file(mPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
    return false;

  QTextStream fout(&file);
  fout << encoded << "\n";
  file.close();

  return true;
}

QString Store::command() const { return ""; }
