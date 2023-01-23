//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TREEMODEL
#define TREEMODEL

#include "git/Diff.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "git/Repository.h"
#include <QAbstractItemModel>
#include <QFileIconProvider>

class TreeModel : public QAbstractItemModel {
  Q_OBJECT

public:
  enum Role {
    BlobRole = Qt::UserRole,
    KindRole,
    AddedRole,
    ModifiedRole,
    StatusRole
  };

  TreeModel(const git::Repository &repo, QObject *parent = nullptr);
  virtual ~TreeModel();

  void setTree(const git::Tree &tree, const git::Diff &diff = git::Diff());

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

  QModelIndex parent(const QModelIndex &index) const override;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  /*!
   * Setting the data to the item
   * \brief setData
   * \param index
   * \param value
   * \param role
   * \param ignoreIndexChanges If index changes should be ignored or not.
   * In normal case it is desired that the index is changed when checking an
   * item, but when the data was changed outside of the model (like in the
   * DiffView) the index must not be updated anymore because it is done already.
   * \return
   */
  bool setData(const QModelIndex &index, const QVariant &value, int role,
               bool ignoreIndexChanges = false);

  Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:
  void checkStateChanged(const QModelIndex &index, int state);

private:
  class Node // item of the model
  {
  public:
    Node(const QString &name, const git::Object &obj, Node *parent = nullptr);
    ~Node();

    QString name() const;
    QString path(bool relative = false) const;

    Node *parent() const;
    bool hasChildren() const;
    QList<Node *> children();

    git::Object object() const;

  private:
    QString mName;
    git::Object mObject;

    Node *mParent{nullptr};
    QList<Node *> mChildren;
  };

  Node *node(const QModelIndex &index) const;

  Node *mRoot = nullptr;
  QFileIconProvider mIconProvider;

  git::Diff mDiff;
  git::Repository mRepo;
};

#endif
