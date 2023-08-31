//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef DIFFTREEMODEL
#define DIFFTREEMODEL

#include "git/Diff.h"
#include "git/Index.h"
#include "git/Tree.h"
#include "git/Repository.h"
#include <QStandardItemModel>
#include <QAbstractListModel>
#include <QFileIconProvider>
#include "git/Index.h"

class Node : public QObject // item of the model
{
  Q_OBJECT

public:
  Node(const QString &name, int patchIndex, Node *parent = nullptr);
  ~Node();

  enum class ParentStageState { Any, Staged, Unstaged };

  QString name() const;
  QString path(bool relative = false) const;

  Node *parent() const;
  bool hasChildren() const;
  QList<Node *> children() const;
  void addChild(const QStringList &pathPart, int patchIndex,
                int indexFirstDifferent, bool listView);
  git::Index::StagedState stageState(const git::Index &idx,
                                     ParentStageState searchingState) const;
  void childFiles(QStringList &files);
  /*!
   * \brief fileCount
   * Counts all files below this node if this node is
   * a folder, otherwise it returns 1 for the current
   * node
   * \return
   */
  int fileCount() const;
  /*!
   * \brief patchIndex
   * \return Current patch index. -1 means the node
   * is a folder and so the index is not valid
   */
  int patchIndex() const;
  /*!
   * \brief patchIndices
   * Get all patch indices from the current Node
   * and all his childs. If a Node is a folder,
   * his index is -1 and then recursive all childs
   * are checked and stored in \p list if they are
   * valid indices (>=0)
   * \param list Contains all patch indices
   */
  void patchIndices(QList<int> &list);
  Node *child(const QStringList &name, int listIndex);

private:
  QString mName;
  /*!
   * Index of the patch in the diff
   */
  int mPatchIndex{-1};
  Node *mParent{nullptr};
  QList<Node *> mChildren;
};

/*!
 * \brief The DiffTreeModel class
 * This Treemodel is similar to the normal tree model, but handles only the
 * files in the diff it self and not the complete tree
 */
class DiffTreeModel : public QStandardItemModel {
  Q_OBJECT

public:
  enum Role {
    BlobRole = Qt::UserRole,
    KindRole,
    AddedRole,
    ModifiedRole,
    StatusRole,
    PatchIndexRole,
  };

  DiffTreeModel(const git::Repository &repo, QObject *parent = nullptr);
  virtual ~DiffTreeModel();

  void setDiff(const git::Diff &diff = git::Diff());
  void refresh(const QStringList &paths);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
  int fileCount(const QModelIndex &parent = QModelIndex()) const;
  QList<int> patchIndices(const QModelIndex &parent) const;

  QModelIndex parent(const QModelIndex &index) const override;
  QList<QModelIndex> modelIndices(const QModelIndex &parent = QModelIndex(),
                                  bool recursive = true) const;
  void modelIndices(const QModelIndex &parent, QList<QModelIndex> &list,
                    bool recursive = true) const;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex index(Node *n) const;
  QModelIndex index(const QString &name) const;

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;

  void createDiffTree();
  /*!
   * \brief discard
   * Discard file or folder
   * \param index
   * \return
   */
  bool discard(const QModelIndex &index);
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
               bool ignoreIndexChanges);

  Qt::ItemFlags flags(const QModelIndex &index) const override;

  void enableListView(bool enable) { mListView = enable; }

signals:
  void checkStateChanged(const QModelIndex &index, int state);
  void updateSubmodules(const QList<git::Submodule> &submodules, bool recursive,
                        bool init, bool checkout_force);

private:
  void handleDataChanged(const QModelIndex &index, int role);

private:
  Node *node(const QModelIndex &index) const;
  QVariant getDisplayRole(const QModelIndex &index) const;

  QFileIconProvider mIconProvider;

  git::Diff mDiff;
  Node *mRoot{nullptr};
  git::Repository mRepo;

  bool mListView = false;
};

#endif /* DIFFTREEMODEL */
