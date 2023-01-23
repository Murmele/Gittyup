//
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef TREEPROXY_H
#define TREEPROXY_H

#include "git/Diff.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "git/Repository.h"
#include <QSortFilterProxyModel>
#include <QFileIconProvider>

class TreeModel;

class TreeProxy : public QSortFilterProxyModel {
  Q_OBJECT

public:
  TreeProxy(bool staged, QObject *parent = nullptr);
  virtual ~TreeProxy();
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole, bool ignoreIndexChanges = false);
  bool staged() { return mStaged; }

  void enableFilter(bool enable) { mFilter = enable; }

private:
  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override;
  bool mStaged{
      true}; // indicates, if only staged or only unstages files should be shown
  bool mFilter = true;
};

#endif // TREEPROXY_H
