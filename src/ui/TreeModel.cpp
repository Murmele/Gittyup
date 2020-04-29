//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "TreeModel.h"
#include "conf/Settings.h"
#include "git/Blob.h"
#include "git/Diff.h"
#include "git/RevWalk.h"
#include "git/Submodule.h"
#include <QStringBuilder>
#include <QUrl>

namespace {

const QString kLinkFmt = "<a href='%1'>%2</a>";

} // anon. namespace

TreeModel::TreeModel(const git::Repository &repo, QObject *parent)
  : QAbstractItemModel(parent), mRepo(repo)
{}

TreeModel::~TreeModel()
{
  delete mRoot;
}

void TreeModel::setTree(const git::Tree &tree, const git::Diff &diff)
{
  beginResetModel();

  delete mRoot;
  mRoot = tree.isValid() ? new Node(mRepo.workdir().path(), tree) : nullptr;

  mDiff = diff;

  endResetModel();
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
  return mRoot ? node(parent)->children().size() : 0;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}

bool TreeModel::hasChildren(const QModelIndex &parent) const
{
  return mRoot && node(parent)->hasChildren();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
  Node *parent = node(index)->parent();
  if (!parent || parent == mRoot)
    return QModelIndex();

  Q_ASSERT(parent->parent()); // because parent is not root
  return createIndex(parent->parent()->children().indexOf(parent), 0, parent);
}

QModelIndex TreeModel::index(
  int row,
  int column,
  const QModelIndex &parent) const
{
  if (row < 0 || row >= rowCount(parent) ||
      column < 0 || column >= columnCount(parent))
    return QModelIndex();

  return createIndex(row, column, node(parent)->children().at(row));
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  Node *node = this->node(index);
  switch (role) {
    case Qt::DisplayRole:
      return node->name();

    case Qt::DecorationRole: {
      QFileInfo info(node->path());
      return info.exists() ? mIconProvider.icon(info) :
        mIconProvider.icon(QFileIconProvider::File);
    }

    case Qt::EditRole:
      return node->path(true);

    case Qt::ToolTipRole:
      return node->path();

    case Qt::CheckStateRole: {
      if (!mDiff.isValid() || !mDiff.isStatusDiff())
        return QVariant();

      QStringList paths;
      QString prefix = node->path(true);
	  // from these files the checkstate for the folders is created,
	  // because the folder it self cannot be staged
      for (int i = 0; i < mDiff.count(); ++i) {
        QString path = mDiff.name(i);
        if (path.startsWith(prefix))
          paths.append(path);
      }

      if (paths.isEmpty())
        return QVariant();

      int count = 0;
      git::Index index = mDiff.index();
      foreach (const QString &path, paths) {
		// isStaged on folders does not work, because folder cannot be staged
        switch (index.isStaged(path)) {
          case git::Index::Disabled:
          case git::Index::Unstaged:
          case git::Index::Conflicted:
            break;

          case git::Index::PartiallyStaged:
            return Qt::PartiallyChecked;
          case git::Index::Staged:
            ++count;
            break;
        }
      }

      if (count == 0) {
        return Qt::Unchecked;
      } else if (count == paths.size()) {
        return Qt::Checked;
      } else {
        return Qt::PartiallyChecked;
      }
    }

    case BlobRole:
      return QVariant::fromValue(git::Blob(node->object()));

    case KindRole: {
      Settings *settings = Settings::instance();
      git::Submodule submodule = mRepo.lookupSubmodule(node->path(true));
      return submodule.isValid() ? tr("Submodule") : settings->kind(node->name());
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
      QString prefix = node->path(true);
      for (int i = 0; i < mDiff.count(); ++i) {
        QString name = mDiff.name(i);
        if (containsPath(name, prefix)) {
          QChar ch = git::Diff::statusChar(mDiff.status(i));
          if (!status.contains(ch))
            status.append(ch);
        }
      }

      return status;
    }
  }

  return QVariant();
}

bool TreeModel::setData(const QModelIndex &index,
                        const QVariant &value,
                        int role)
{
    setData(index, value, role, false);
}

bool TreeModel::setData(const QModelIndex &index,
                        const QVariant &value,
                        int role,
                        bool ignoreIndexChanges)
{
  switch (role) {
    case Qt::CheckStateRole: {
      QStringList files;
      Node *node = this->node(index);
      QString prefix = node->path(true);
      for (int i = 0; i < mDiff.count(); ++i) {
        QString file = mDiff.name(i);
        if (file.startsWith(prefix))
          files.append(file);
      }

      if (!ignoreIndexChanges)
        mDiff.index().setStaged(files, value.toBool());

	  // childs
	  if (hasChildren(index)) {
		  // emit dataChanged() for all files in the folder
		  // all children changed too. TODO: only the tracked files should emit a signal
		  int count = rowCount(index);
		  for (int row = 0; row < count; row++) {
			  QModelIndex child = this->index(row, 0, index);
			  emit dataChanged(child, child, {role});
		  }
	  }
	  // parents
	  // recursive approach to emit signal dataChanged also for the parents.
	  // Because when a file in a folder is staged, the state of the folder changes too
	  QModelIndex parent = this->parent(index);
	  while (parent.isValid()) {
		  emit dataChanged(parent, parent, {role});
		  parent = this->parent(parent);
	  }

	  // file/folder it self
	  // emit dataChanged() for folder or file it self
	  emit dataChanged(index, index, {role});
      emit checkStateChanged(index, value.toInt());

      return true;
    }
  }

  return false;
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
  return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
}

TreeModel::Node *TreeModel::node(const QModelIndex &index) const
{
  return index.isValid() ? static_cast<Node *>(index.internalPointer()) : mRoot;
}

//#############################################################################
//######     Treemodel::Node     ##############################################
//#############################################################################

TreeModel::Node::Node(const QString &name, const git::Object &obj, Node *parent)
  : mName(name), mObject(obj), mParent(parent)
{}

TreeModel::Node::~Node()
{
  qDeleteAll(mChildren);
}

QString TreeModel::Node::name() const
{
  return mName;
}

QString TreeModel::Node::path(bool relative) const
{
  bool root = (!mParent || (relative && !mParent->mParent));
  return !root ? mParent->path(relative) % "/" % mName : mName;
}

TreeModel::Node *TreeModel::Node::parent() const
{
  return mParent;
}

bool TreeModel::Node::hasChildren() const
{
  git::Tree tree = object();
  return (tree.isValid() && tree.count() > 0);
}

QList<TreeModel::Node *> TreeModel::Node::children()
{
  if (mChildren.isEmpty() && hasChildren()) {
    git::Tree tree = object();
    for (int i = 0; i < tree.count(); ++i) {
        QString name = tree.name(i);
      mChildren.append(new Node(tree.name(i), tree.object(i), this));
    }
  }

  return mChildren;
}

git::Object TreeModel::Node::object() const
{
  return mObject;
}
