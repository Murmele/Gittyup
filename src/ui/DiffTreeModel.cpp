//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DiffTreeModel.h"
#include "conf/Settings.h"
#include "git/Blob.h"
#include "git/Diff.h"
#include "git/RevWalk.h"
#include "git/Submodule.h"
#include "git/Patch.h"
#include <QStringBuilder>
#include <QUrl>

namespace {

const QString kLinkFmt = "<a href='%1'>%2</a>";

} // namespace

DiffTreeModel::DiffTreeModel(const git::Repository &repo, QObject *parent)
    : QAbstractItemModel(parent), mRepo(repo) {}

DiffTreeModel::~DiffTreeModel() { delete mRoot; }

void DiffTreeModel::createDiffTree() {

  for (int patchNum = 0; patchNum < mDiff.count(); ++patchNum) {
    QString path = mDiff.name(patchNum);
    auto pathParts = path.split("/");
    mRoot->addChild(pathParts, patchNum, 0, mListView);
  }
}

void DiffTreeModel::setDiff(const git::Diff &diff) {
  beginResetModel();

  if (diff) {
    delete mRoot;
    mDiff = diff;
    mRoot = new Node(mRepo.workdir().path(), -1);
    createDiffTree();
  }

  endResetModel();
}

void DiffTreeModel::refresh(const QStringList &paths) {
  for (auto path : paths) {
    auto index = this->index(path);
    handleDataChanged(index, Qt::CheckStateRole);
  }
}

void DiffTreeModel::handleDataChanged(const QModelIndex &index, int role) {
  if (!index.isValid())
    return;

  // childs
  if (hasChildren(index)) {
    // emit dataChanged() for all files in the folder
    // all children changed too. TODO: only the tracked files should emit a
    // signal
    int count = rowCount(index);
    for (int row = 0; row < count; row++) {
      QModelIndex child = this->index(row, 0, index);
      emit dataChanged(child, child, {role});
    }
  }
  // parents
  // recursive approach to emit signal dataChanged also for the parents.
  // Because when a file in a folder is staged, the state of the folder
  // changes too
  QModelIndex parent = this->parent(index);
  while (parent.isValid()) {
    emit dataChanged(parent, parent, {role});
    parent = this->parent(parent);
  }

  // file/folder it self
  // emit dataChanged() for folder or file it self
  emit dataChanged(index, index, {role});
}

int DiffTreeModel::rowCount(const QModelIndex &parent) const {
  return mDiff ? node(parent)->children().size() : 0;
}

int DiffTreeModel::columnCount(const QModelIndex &parent) const { return 1; }

bool DiffTreeModel::hasChildren(const QModelIndex &parent) const {
  return mRoot && node(parent)->hasChildren();
}

int DiffTreeModel::fileCount(const QModelIndex &parent) const {
  if (!mRoot)
    return 0;

  return node(parent)->fileCount();
}

QList<int> DiffTreeModel::patchIndices(const QModelIndex &parent) const {
  QList<int> list;
  if (!mRoot)
    return list;

  node(parent)->patchIndices(list);
  return list;
}

QModelIndex DiffTreeModel::parent(const QModelIndex &index) const {
  Node *parent = node(index)->parent();
  if (!parent || parent == mRoot)
    return QModelIndex();

  Q_ASSERT(parent->parent()); // because parent is not root
  return createIndex(parent->parent()->children().indexOf(parent), 0, parent);
}

void DiffTreeModel::modelIndices(const QModelIndex &parent,
                                 QList<QModelIndex> &list,
                                 bool recursive) const {
  // TODO: Method name a little bit missleading, because also the
  // parent is added
  Node *n = node(parent);
  if (!n->hasChildren()) {
    list.append(index(n));
    return;
  }

  for (int i = 0; i < n->children().length(); i++) {
    auto child = createIndex(i, 0, n->children()[i]);
    if (recursive)
      modelIndices(child, list);
    else if (!node(child)->hasChildren())
      list.append(child);
  }
}

QList<QModelIndex> DiffTreeModel::modelIndices(const QModelIndex &parent,
                                               bool recursive) const {
  QList<QModelIndex> childs;

  if (!parent.isValid())
    return childs;

  modelIndices(parent, childs, recursive);

  return childs;
}

QModelIndex DiffTreeModel::index(int row, int column,
                                 const QModelIndex &parent) const {
  if (row < 0 || row >= rowCount(parent) || column < 0 ||
      column >= columnCount(parent))
    return QModelIndex();

  auto n = node(parent)->children().at(row);
  return createIndex(row, column, n);
}

QModelIndex DiffTreeModel::index(Node *n) const {
  int row = n->parent()->children().indexOf(n);
  return createIndex(row, 0, n);
}

QModelIndex DiffTreeModel::index(const QString &name) const {
  auto list = name.split("/");
  if (list.length() == 0)
    return QModelIndex();

  for (auto c : mRoot->children()) {
    auto n = c->child(list, 0);
    if (n)
      return index(n);
  }

  return QModelIndex();
}

QVariant DiffTreeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  Node *node = this->node(index);
  switch (role) {
    case Qt::DisplayRole:
      return node->name();

      //    case Qt::DecorationRole: {
      //      QFileInfo info(node->path());
      //      return info.exists() ? mIconProvindexider.icon(info) :
      //        mIconProvider.icon(QFileIconProvider::File);
      //    }

    case Qt::EditRole:
      return node->path(true);

    case Qt::ToolTipRole:
      return node->path();

    case Qt::CheckStateRole: {
      if (!mDiff.isValid() || !mDiff.isStatusDiff())
        return QVariant();

      git::Index index = mDiff.index();
      const auto state = node->stageState(index, Node::ParentStageState::Any);
      switch (state) {
        case git::Index::StagedState::PartiallyStaged:
          return Qt::CheckState::PartiallyChecked;
        case git::Index::StagedState::Staged:
          return Qt::CheckState::Checked;
        case git::Index::StagedState::Unstaged:
          return Qt::CheckState::Unchecked;
        case git::Index::StagedState::Conflicted:
        // fall through
        case git::Index::StagedState::Disabled: // dirty submodules
          return Qt::CheckState::Unchecked;
      }

      return Qt::CheckState::Unchecked;
    }

    case KindRole: {
      Settings *settings = Settings::instance();
      git::Submodule submodule = mRepo.lookupSubmodule(node->path(true));
      return submodule.isValid() ? tr("Submodule")
                                 : settings->kind(node->name());
    }

    case AddedRole:
    case ModifiedRole: {
      int sort = GIT_SORT_TIME;
      if (role == AddedRole)
        sort |= GIT_SORT_REVERSE;
      git::RevWalk walker = mRepo.walker(sort);
      git::Commit commit = walker.next(node->path(true));
      if (!commit.isValid())
        return QVariant();

      QUrl url;
      url.setScheme("id");
      url.setPath(commit.id().toString());
      return kLinkFmt.arg(url.toString(), commit.shortId());
    }

    case StatusRole: {
      if (!mDiff.isValid())
        return QString();

      QString status;
      QList<int> patchIndices;
      node->patchIndices(patchIndices);
      for (auto patchIndex : patchIndices) {
        QChar ch = git::Diff::statusChar(mDiff.status(patchIndex));
        if (!status.contains(ch))
          status.append(ch);
      }

      return status;
    }
    case PatchIndexRole: {
      return node->patchIndex();
    }

    case Qt::UserRole:
      return QVariant::fromValue(node);
  }

  return QVariant();
}

bool DiffTreeModel::setData(const QModelIndex &index, const QVariant &value,
                            int role) {
  return setData(index, value, role, false);
}

bool DiffTreeModel::discard(const QModelIndex &index) {
  assert(index.isValid());

  Node *node = this->node(index);
  QList<int> list;
  node->patchIndices(list);

  QStringList trackedPatches;
  for (auto i : list) {
    auto patch = mDiff.patch(i);
    if (patch.isUntracked()) {
      QString name = patch.name();
      git::Repository repo = patch.repo();
      QDir dir = repo.workdir();
      if (QFileInfo(dir.filePath(name)).isDir()) {
        if (dir.cd(name))
          dir.removeRecursively();
      } else {
        dir.remove(name);
      }
    } else {
      trackedPatches.append(patch.name());
    }
  }

  auto s = mRepo.submodules();
  QList<git::Submodule> submodules;
  QStringList filePatches;
  for (auto trackedPatch : trackedPatches) {
    bool is_submodule = false;
    for (auto submodule : s) {
      if (submodule.path() == trackedPatch) {
        is_submodule = true;
        submodules.append(submodule);
        break;
      }
    }
    emit updateSubmodules(submodules, true, false, true);

    if (!is_submodule)
      filePatches.append(trackedPatch);
  }

  if (filePatches.length() > 0) {
    int strategy = GIT_CHECKOUT_FORCE;
    auto repo = mDiff.patch(list[0]).repo(); // does not matter which index is
                                             // used all are in the same repo
    if (!repo.checkout(git::Commit(), nullptr, trackedPatches, strategy))
      return false;
  }
  return true;
}

bool DiffTreeModel::setData(const QModelIndex &index, const QVariant &value,
                            int role, bool ignoreIndexChanges) {
  switch (role) {
    case Qt::CheckStateRole: {
      QStringList files;
      Node *node = this->node(index);

      if (!ignoreIndexChanges) {
        QStringList files;
        node->childFiles(files);
        git::Index::StagedState state =
            static_cast<git::Index::StagedState>(value.toInt());
        if (state == git::Index::StagedState::Staged)
          mDiff.index().setStaged(files, true);
        else if (state == git::Index::StagedState::Unstaged)
          mDiff.index().setStaged(files, false);
        else if (state == git::Index::StagedState::PartiallyStaged)
          // is done directly in the hunkwidget, because it gets to complicated
          // to do line staging here.
          (void)state;
      }

      handleDataChanged(index, role);
      emit checkStateChanged(index, value.toInt());

      return true;
    }
  }

  return false;
}

Qt::ItemFlags DiffTreeModel::flags(const QModelIndex &index) const {
  return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
}

Node *DiffTreeModel::node(const QModelIndex &index) const {
  return index.isValid() ? static_cast<Node *>(index.internalPointer()) : mRoot;
}

//#############################################################################
//######     DiffTreeModel::Node ##############################################
//#############################################################################

Node::Node(const QString &name, int patchIndex, Node *parent)
    : mName(name), mPatchIndex(patchIndex), mParent(parent) {}

Node::~Node() { qDeleteAll(mChildren); }

QString Node::name() const { return mName; }

QString Node::path(bool relative) const {
  bool root = (!mParent || (relative && !mParent->mParent));
  return !root ? mParent->path(relative) % "/" % mName : mName;
}

Node *Node::parent() const { return mParent; }

bool Node::hasChildren() const { return mChildren.length() > 0; }

QList<Node *> Node::children() const { return mChildren; }

void Node::addChild(const QStringList &pathPart, int patchIndex,
                    int indexFirstDifferent, bool listView) {
  Node *node = nullptr;

  if (listView) {
    // Add child with full path in list view.
    node = new Node(pathPart.join("/"), patchIndex, this);
  } else {
    for (auto child : mChildren) {
      if (child->name() == pathPart[indexFirstDifferent]) {
        child->addChild(pathPart, patchIndex, indexFirstDifferent + 1, false);
        return;
      }
    }

    if (indexFirstDifferent + 1 < pathPart.length()) {
      // Folders cannot have a patch Index!
      node = new Node(pathPart[indexFirstDifferent], -1, this);
      node->addChild(pathPart, patchIndex, indexFirstDifferent + 1, false);
    } else if (indexFirstDifferent < pathPart.length())
      node = new Node(pathPart[indexFirstDifferent], patchIndex, this);
  }
  if (node)
    mChildren.append(node);
}

git::Index::StagedState
Node::stageState(const git::Index &idx, ParentStageState searchingState) const {
  if (!hasChildren())
    return idx.isStaged(path(true));

  git::Index::StagedState childState;
  for (auto child : mChildren) {

    childState = child->stageState(idx, searchingState);

    // determine new searching state
    // conflicted and disabled files/submodules are in the unstaged treeview,
    if ((childState == git::Index::StagedState::Staged &&
         searchingState == ParentStageState::Unstaged) ||
        ((childState == git::Index::StagedState::Unstaged ||
          childState == git::Index::StagedState::Conflicted ||
          childState == git::Index::StagedState::Disabled) &&
         searchingState == ParentStageState::Staged) ||
        childState == git::Index::PartiallyStaged)
      return git::Index::PartiallyStaged;

    if (searchingState == ParentStageState::Any) {
      switch (childState) {
        case git::Index::Unstaged:
          searchingState = ParentStageState::Unstaged;
          break;
        case git::Index::Staged:
          searchingState = ParentStageState::Staged;
          break;
        case git::Index::PartiallyStaged:
          break; // does not change searchingState
        case git::Index::Conflicted:
          // fall through
        case git::Index::Disabled:
          searchingState = ParentStageState::Unstaged;
          break;
      }
    }
  }

  return childState;
}

void Node::childFiles(QStringList &files) {
  if (!hasChildren()) {
    files.append(path(true));
    return;
  }

  for (auto child : mChildren)
    child->childFiles(files);
}

int Node::fileCount() const {
  if (!hasChildren())
    return 1;

  int count = 0;
  for (auto child : mChildren)
    count += child->fileCount();

  return count;
}

void Node::patchIndices(QList<int> &list) {
  if (!hasChildren()) {
    assert(patchIndex() >= 0);
    list.append(patchIndex());
    return;
  }

  for (auto child : mChildren)
    child->patchIndices(list);
}

int Node::patchIndex() const { return mPatchIndex; }

Node *Node::child(const QStringList &name, int listIndex) {

  if (name[listIndex] != mName)
    return nullptr;

  if (listIndex >= name.length() - 1) { // > should not occur
    return this;
  }

  for (auto c : mChildren) {
    Node *n = c->child(name, listIndex + 1);
    if (n != nullptr)
      return n;
  }
  return nullptr;
}
