//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef QUERY_H
#define QUERY_H

#include "Index.h"
#include <QSharedPointer>

using QueryRef = QSharedPointer<class Query>;

class Query {
public:
  virtual ~Query() {}

  virtual QString toString() const = 0;
  virtual QList<Index::Term> terms() const = 0;
  virtual QList<git::Commit> commits(const Index *index) const = 0;

  static QueryRef parseQuery(const QString &query);
};

#endif
