#include "ReferenceModel.h"
#include "Debug.h"
#include "git/Signature.h"
#include "git/Tag.h"
#include "git/Branch.h"
#include "git/TagRef.h"

#include <QDateTime>

namespace {
const QString kNowrapFmt = "<span style='white-space: nowrap'>%1</span>";

bool refComparator(const git::Reference &lhs, const git::Reference &rhs) {
  git::Commit lhsCommit = lhs.target();
  git::Commit rhsCommit = rhs.target();
  return (lhsCommit.isValid() && rhsCommit.isValid() &&
          lhsCommit.committer().date() > rhsCommit.committer().date());
}
} // namespace

ReferenceModel::ReferenceModel(const git::Repository &repo,
                               ReferenceView::Kinds kinds, QObject *parent)
    : QAbstractItemModel(parent), mRepo(repo), mKinds(kinds) {
  if (repo.isValid()) {
    git::RepositoryNotifier *notifier = repo.notifier();
    connect(notifier, &git::RepositoryNotifier::referenceAdded, this,
            &ReferenceModel::update);
    connect(notifier, &git::RepositoryNotifier::referenceRemoved, this,
            &ReferenceModel::update);
    connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
            &ReferenceModel::update);
  }
}

int ReferenceModel::referenceTypeToIndex(ReferenceType t) const {
  int index = t;
  // No local branches header exists, so the index must be lowered
  if (!(mKinds & ReferenceView::Kind::LocalBranches)) {
    index--;
  }

  // No remote branches header exists, so the index must be lowered
  if (t != ReferenceType::Branches &&
      !(mKinds & ReferenceView::Kind::RemoteBranches)) {
    index--;
  }
  return index - 1;
}

int ReferenceModel::indexToReferenceType(int index) const {
  int type = index + 1;
  // No local branches header exists, so the index must be lowered
  if (!(mKinds & ReferenceView::Kind::LocalBranches)) {
    type++;
  }

  // No remote branches header exists, so the index must be lowered
  if (type != ReferenceType::Branches &&
      !(mKinds & ReferenceView::Kind::RemoteBranches)) {
    type++;
  }
  return static_cast<ReferenceType>(type);
}

void ReferenceModel::setCommit(const git::Commit &commit) {
  mCommit = commit;
  update();
}

void ReferenceModel::update() {
  beginResetModel();

  mRefs.clear();

  // Add detached head.
  git::Reference detachedHead;
  if (mKinds & ReferenceView::DetachedHead) {
    git::Reference head = mRepo.head();
    if (head.isValid() && !head.isBranch()) {
      if (!mCommit.isValid() || head.annotatedCommit().commit() == mCommit) {
        detachedHead = head;
      }
    }
  }

  // Add local branches.
  if (mKinds & ReferenceView::LocalBranches) {
    QList<git::Reference> branches;
    foreach (const git::Branch &branch, mRepo.branches(GIT_BRANCH_LOCAL)) {
      //Debug("ReferenceView: Local branches: " << branch.name());
      const bool branchOnCommit =
          !mCommit.isValid() || branch.annotatedCommit().commit() == mCommit;
      if ((!(mKinds & ReferenceView::ExcludeHead) || !branch.isHead()) &&
          branchOnCommit)
        branches.append(branch);
    }

    std::sort(branches.begin(), branches.end(), refComparator);

    // Add top references.
    if (detachedHead.isValid())
      branches.prepend(detachedHead);
    if (mKinds & ReferenceView::InvalidRef)
      branches.prepend(git::Reference());

    // Add bottom references.
    const bool stashOnCommit =
        !mCommit.isValid() ||
        mRepo.stashRef().annotatedCommit().commit() == mCommit;
    if ((mKinds & ReferenceView::Stash) && stashOnCommit) {
      if (git::Reference stash = mRepo.stashRef())
        branches.append(stash);
    }

    mRefs.append({tr("Branches"), branches,
                  ReferenceType::Branches}); // First element in mRefs
  }

  // Add remote branches.
  if (mKinds & ReferenceView::RemoteBranches) {
    QList<git::Reference> remotes;
    foreach (const git::Branch &branch, mRepo.branches(GIT_BRANCH_REMOTE)) {
      // Filter remote HEAD branches.
      Debug("ReferenceView: Remote branches: " << branch.name());
      const bool remoteOnCommit =
          !mCommit.isValid() || branch.annotatedCommit().commit() == mCommit;
      if (!branch.name().endsWith("HEAD") && remoteOnCommit)
        remotes.append(branch);
    }

    std::sort(remotes.begin(), remotes.end(), refComparator);
    if (mKinds & ReferenceView::InvalidRef)
      remotes.prepend(git::Reference());
    mRefs.append({tr("Remotes"), remotes,
                  ReferenceType::Remotes}); // Second element in mRefs
  }

  // Add tags.
  if (mKinds & ReferenceView::Tags) {
    QList<git::Reference> tags;
    foreach (const git::TagRef &tag, mRepo.tags()) {
      Debug("ReferenceView: Tags: " << tag.name());
      const bool tagOnCommit =
          !mCommit.isValid() || tag.annotatedCommit().commit() == mCommit;
      if (tagOnCommit)
        tags.append(tag);
    }

    std::sort(tags.begin(), tags.end(), refComparator);
    mRefs.append(
        {tr("Tags"), tags, ReferenceType::Tags}); // Third element in mRefs
  }

  endResetModel();
}

QModelIndex ReferenceModel::firstRemote() {
  // Return first remote if available, otherwise an invalid modelIndex
  for (auto &ref : mRefs) {
    if (ref.type == ReferenceType::Remotes && ref.refs.count() > 0) {
      if (mKinds & ReferenceView::InvalidRef) {
        if (ref.refs.count() > 1) {
          // use the first valid ref after the invalid ref
          return createIndex(1, 0, ReferenceType::Remotes);
        }
      } else
        return createIndex(0, 0, ReferenceType::Remotes);
      break; // Remotes are already found so no need to continue searching
    }
  }
  return QModelIndex();
}

QModelIndex ReferenceModel::firstBranch() {
  // Return first branch if available, otherwise an invalid modelIndex
  // Ignore stash
  for (auto &ref : mRefs) {
    if (ref.type == ReferenceType::Branches && ref.refs.count() > 0) {
      if (mKinds & ReferenceView::InvalidRef) {
        // use the first valid ref after the invalid ref but only
        // it is not the stash
        if (ref.refs.count() > 1 &&
            ref.refs.at(1).annotatedCommit().commit() !=
                mRepo.stashRef().annotatedCommit().commit())
          return createIndex(1, 0, ReferenceType::Branches);
      } else if (ref.refs.count() > 0)
        return createIndex(0, 0, ReferenceType::Branches);
      break; // Remotes are already found so no need to continue searching
    }
  }
  return QModelIndex();
}

QModelIndex ReferenceModel::firstTag() {
  // Return first tag if available, otherwise an invalid modelIndex

  for (auto &ref : mRefs) {
    if (ref.type == ReferenceType::Tags && ref.refs.count() > 0)
      return createIndex(0, 0, ReferenceType::Tags);
  }
  return QModelIndex();
}

QModelIndex ReferenceModel::index(int row, int column,
                                  const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  // headers do not have a parent
  bool header = !parent.isValid();
  return createIndex(row, column,
                     header ? 0 : indexToReferenceType(parent.row()));
}

QModelIndex ReferenceModel::parent(const QModelIndex &index) const {
  if (!index.isValid())
    return QModelIndex();

  // The header sections (Branches, Remotes, Tags) (Elements of mRefs)
  // do not have any parents, only the refs in each mRefs element
  const quintptr id = index.internalId();
  if (id == COMBOBOX_HEADER)
    return QModelIndex();
  const auto row = referenceTypeToIndex(static_cast<ReferenceType>(id));
  return createIndex(row, 0);
}

int ReferenceModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid())
    return mRefs.size();

  if (parent.internalId() != COMBOBOX_HEADER)
    return 0; // refs it self do not have childs

  return mRefs.at(parent.row()).refs.size();
}

int ReferenceModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant ReferenceModel::data(const QModelIndex &index, int role) const {
  // kinds
  int row = index.row();
  quintptr id = index.internalId();
  if (id == COMBOBOX_HEADER)
    return (role == Qt::DisplayRole) ? mRefs.at(row).name : QVariant();

  auto refType = static_cast<ReferenceType>(id);

  // refs
  git::Reference ref = mRefs.at(referenceTypeToIndex(refType)).refs.at(row);
  switch (role) {
    case Qt::DisplayRole:
      return ref.isValid() ? ref.name() : QString();

    case Qt::ToolTipRole: {
      if (!ref.isValid() || !ref.isTag())
        return QVariant();

      git::Tag tag = git::TagRef(ref).tag();
      if (!tag.isValid())
        return QVariant();

      QStringList lines;
      if (git::Signature signature = tag.tagger()) {
        QString name = QString("<b>%1</b>").arg(signature.name());
        QString email = QString("&lt;%1&gt;").arg(signature.email());
        lines.append(kNowrapFmt.arg(QString("%1 %2").arg(name, email)));

        QString date =
            QLocale().toString(signature.date(), QLocale::LongFormat);
        lines.append(kNowrapFmt.arg(date));
      }

      QString msg = tag.message();
      if (!msg.isEmpty())
        lines.append(QString("<p>%1</p>").arg(msg));

      return lines.join('\n');
    }

    case Qt::FontRole: {
      QFont font = static_cast<QWidget *>(QObject::parent())->font();
      font.setBold(ref.isValid() && ref.isHead());
      return font;
    }

    case Qt::UserRole:
      return QVariant::fromValue(ref);

    default:
      return QVariant();
  }
}

QVariant ReferenceModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  return QVariant();
}
