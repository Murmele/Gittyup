//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef HOST_REPOSITORY_H
#define HOST_REPOSITORY_H

#include <QMap>
#include <QMetaType>
#include <QSharedPointer>
#include <QString>

class Account;

class Repository : public QObject {
  Q_OBJECT

public:
  enum Protocol { Https, Ssh };

  Repository(const QString &name, const QString &fullName,
             Account *parent = nullptr);

  Account *account() const;

  QString name() const { return mName; }
  QString owner() const { return mFullName.section('/', 0, 0); }
  QString fullName() const { return mFullName; }

  QString url(Protocol protocol) const { return mUrls.value(protocol); }
  void setUrl(Protocol protocol, const QString &url) { mUrls[protocol] = url; }

private:
  // remote properties
  QString mName;
  QString mFullName;
  QMap<Protocol, QString> mUrls;
};

#endif
