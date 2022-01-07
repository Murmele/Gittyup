//
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef TREEPROXY_H
#define TREEPROXY_H

#include <QSortFilterProxyModel>

class TreeProxy : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  TreeProxy(bool staged, QObject *parent = nullptr);
  virtual ~TreeProxy();

private:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
  bool mStaged = true; // indicates, if only staged or only unstages files should be shown
};

#endif // TREEPROXY_H
