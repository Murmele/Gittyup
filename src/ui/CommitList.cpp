//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CommitList.h"
#include "Badge.h"
#include "Location.h"
#include "MainWindow.h"
#include "ProgressIndicator.h"
#include "RepoView.h"
#include "Debug.h"
#include "ConfigKeys.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "dialogs/MergeDialog.h"
#include "index/Index.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "git/Patch.h"
#include "git/RevWalk.h"
#include "git/Signature.h"
#include "git/TagRef.h"
#include "git/Tree.h"
#include "ui/HotkeyManager.h"
#include <QAbstractListModel>
#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QtConcurrent>

namespace {

// FIXME: Factor out into theme?
const QColor kTaintedColor = Qt::gray;

const QString kPathspecFmt = "pathspec:%1";

// Use fixed short id size in compact mode.
// FIXME: Use 'core.abbrev' config instead?
const int kShortIdSize = 7;

enum GraphSegment {
  Dot,
  Top,
  Middle,
  Bottom,
  Cross,
  LeftIn,
  LeftOut,
  RightIn,
  RightOut
};

class DiffCallbacks : public git::Diff::Callbacks {
public:
  void setCanceled(bool canceled) { mCanceled = canceled; }

  bool progress(const QString &oldPath, const QString &newPath) override {
    return !mCanceled;
  }

private:
  bool mCanceled = false;
};

/*!
 * \brief The CommitModel class
 * Model showing all commits as timeline
 */
class CommitModel : public QAbstractListModel {
  Q_OBJECT

public:
  CommitModel(const git::Repository &repo, QObject *parent = nullptr)
      : QAbstractListModel(parent), mRepo(repo) {
    // Connect progress timer.
    connect(&mTimer, &QTimer::timeout, [this] {
      ++mProgress;
      QModelIndex idx = index(0, 0);
      emit dataChanged(idx, idx, {Qt::DisplayRole});
    });

    // Connect watcher to signal when the status diff finishes.
    connect(&mStatus, &QFutureWatcher<git::Diff>::finished, [this] {
      mTimer.stop();
      resetWalker();
      emit statusFinished(!mRows.isEmpty() && !mRows.first().commit.isValid());
    });

    resetSettings();
  }

  git::Reference reference() const { return mRef; }

  git::Diff status() const {
    if (!mStatus.isFinished())
      return git::Diff();

    QFuture<git::Diff> future = mStatus.future();
    if (!future.resultCount())
      return git::Diff();

    return future.result();
  }

  void startStatus() {
    // Cancel existing status diff.
    cancelStatus();

    // Reload the index before starting the status thread. Allowing
    // it to reload on the thread frequently corrupts the index.
    mRepo.index().read();

    // Check for uncommitted changes asynchronously.
    mProgress = 0;
    mTimer.start(50);
    mStatus.setFuture(QtConcurrent::run([this] {
      // Pass the repo's index to suppress reload.
      bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
      return mRepo.status(mRepo.index(), &mStatusCallbacks, ignoreWhitespace);
    }));
  }

  void cancelStatus() {
    if (!mStatus.isRunning())
      return;

    mStatusCallbacks.setCanceled(true);
    mStatus.waitForFinished();
    mStatus.setFuture(QFuture<git::Diff>());
    mStatusCallbacks.setCanceled(false);
  }

  void setPathspec(const QString &pathspec) {
    if (mPathspec == pathspec)
      return;

    mPathspec = pathspec;
    resetWalker();
  }

  void suppressResetWalker(bool suppress) { mSuppressResetWalker = suppress; }

  bool isResetWalkerSuppressed() { return mSuppressResetWalker; }

  void setReference(const git::Reference &ref) {
    mRef = ref;
    if (!mSuppressResetWalker) {
      resetWalker();
    }
  }

  void resetReference(const git::Reference &ref) {
    // Reset selected ref to updated ref.
    if (ref.isValid() && mRef.isValid() &&
        ref.qualifiedName() == mRef.qualifiedName())
      mRef = ref;

    // Status is invalid after HEAD changes.
    if (!ref.isValid() || ref.isHead())
      startStatus();
    else if (!mSuppressResetWalker) {
      // reset walker will be done when status finished
      resetWalker();
    }
  }

  void resetWalker() {
    beginResetModel();

    // Reset state.
    mParents.clear();
    mRows.clear();
    DebugRefresh("");

    // Update status row.
    bool head = (!mRef.isValid() || mRef.isHead());
    bool valid = (!mStatus.isFinished() || status().isValid());
    if (mShowCleanStatus && head && valid && mPathspec.isEmpty()) {
      QVector<Column> row;
      if (mGraphVisible && mRef.isValid() && mStatus.isFinished()) {
        row.append({Segment(Bottom, kTaintedColor), Segment(Dot, QColor())});
        mParents.append(Parent(mRef.target(), nextColor(), true));
      }
      DebugRefresh("mRows append invalid commit");
      mRows.append(Row(git::Commit(), row)); // Uncommitted changes
    }

    // Begin walking commits.
    if (mRef.isValid()) {
      int sort = GIT_SORT_NONE;
      if (mGraphVisible) {
        sort |= GIT_SORT_TOPOLOGICAL;
        if (mSortDate)
          sort |= GIT_SORT_TIME;
      } else if (!mSortDate) {
        sort |= GIT_SORT_TOPOLOGICAL;
      }

      mWalker = mRef.walker(
          sort, mRefsFilter == CommitList::RefsFilter::SelectedRefIgnoreMerge);
      if (mRef.isLocalBranch()) {
        // Add the upstream branch.
        if (git::Branch upstream = git::Branch(mRef).upstream())
          mWalker.push(upstream);
      }

      if (mRef.isHead()) {
        // Add merge head.
        if (git::Reference mergeHead = mRepo.lookupRef("MERGE_HEAD"))
          mWalker.push(mergeHead);
      }

      if (mRefsFilter == CommitList::RefsFilter::AllRefs) {
        foreach (const git::Reference ref, mRepo.refs()) {
          if (!ref.isStash())
            mWalker.push(ref);
        }
      }
    }

    if (canFetchMore(QModelIndex()))
      fetchMore(QModelIndex());

    endResetModel();
  }

  void resetSettings(bool walk = false) {
    git::Config config = mRepo.appConfig();
    mRefsFilter = static_cast<CommitList::RefsFilter>(config.value<int>(
        ConfigKeys::kRefsKey, (int)CommitList::RefsFilter::AllRefs));
    mSortDate = config.value<bool>(ConfigKeys::kSortKey, true);
    mShowCleanStatus = config.value<bool>(ConfigKeys::kStatusKey, true);
    mGraphVisible = config.value<bool>(ConfigKeys::kGraphKey, true);

    if (walk)
      resetWalker();
  }

  bool canFetchMore(const QModelIndex &parent) const {
    return mWalker.isValid();
  }

  void fetchMore(const QModelIndex &parent) {
    // Load commits.
    int i = 0;
    QList<Row> rows;
    git::Commit commit = mWalker.next(mPathspec);
    while (commit.isValid()) {
      // Add root commits.
      bool root = false;
      if (indexOf(commit) < 0) {
        root = true;
        mParents.append(Parent(commit, nextColor()));
      }

      // Calculate graph columns.
      // Remember current row.
      QList<Parent> parents = mParents;

      // Replace commit with its parents.
      QList<git::Commit> replacements;
      foreach (const git::Commit &parent, commit.parents()) {
        // FIXME: Mark commits that point to existing parent?
        if (indexOf(parent) < 0 && !contains(parent, rows))
          replacements.append(parent);
        if (mRefsFilter == CommitList::RefsFilter::SelectedRefIgnoreMerge) {
          break;
        }
      }

      // Set parents for next row.
      int index = indexOf(commit);
      if (index >= 0) {
        Parent parent = mParents.takeAt(index);
        if (!replacements.isEmpty()) {
          git::Commit replacement = replacements.takeFirst();
          mParents.insert(index, Parent(replacement, parent.color));
          foreach (const git::Commit &replacement, replacements)
            mParents.append(Parent(replacement, nextColor()));
        }
      }

      // Add graph row.
      QVector<Column> row;
      if (mGraphVisible && mPathspec.isEmpty())
        row = columns(commit, parents, root);

      rows.append(Row(commit, row));
      DebugRefresh("Append commit: " << commit.shortId());

      // Bail out.
      if (i++ >= 64)
        break;

      commit = mWalker.next(mPathspec);
    }

    // Update the model.
    if (!rows.isEmpty()) {
      int first = mRows.size();
      int last = first + rows.size() - 1;
      beginInsertRows(QModelIndex(), first, last);
      mRows.append(rows);
      endInsertRows();
    }

    // Invalidate walker.
    if (!commit.isValid())
      mWalker = git::RevWalk();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const {
    return mRows.size();
  }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    const Row &row = mRows.at(index.row());
    bool status = !row.commit.isValid();
    switch (role) {
      case Qt::DisplayRole:
        if (!status)
          return QVariant();

        return mStatus.isFinished() ? tr("Uncommitted changes")
                                    : tr("Checking for uncommitted changes");

      case Qt::FontRole: {
        if (!status)
          return QVariant();

        QFont font = static_cast<QWidget *>(QObject::parent())->font();
        font.setItalic(true);
        return font;
      }

      case Qt::TextAlignmentRole:
        if (!status)
          return QVariant();

        return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);

      case Qt::DecorationRole:
        if (!status)
          return QVariant();

        return mStatus.isFinished() ? QVariant() : mProgress;

      case CommitList::Role::DiffRole: {
        if (status)
          return QVariant::fromValue(this->status());

        bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
        git::Diff diff = row.commit.diff(git::Commit(), -1, ignoreWhitespace);
        diff.findSimilar();
        return QVariant::fromValue(diff);
      }

      case CommitList::Role::CommitRole:
        return status ? QVariant() : QVariant::fromValue(row.commit);

      case CommitList::Role::GraphRole: {
        QVariantList columns;
        foreach (const Column &column, row.columns) {
          QVariantList segments;
          foreach (const Segment &segment, column)
            segments.append(segment.segment);
          columns.append(QVariant(segments));
        }

        return columns;
      }

      case CommitList::Role::GraphColorRole: {
        QVariantList columns;
        foreach (const Column &column, row.columns) {
          QVariantList segments;
          foreach (const Segment &segment, column)
            segments.append(segment.color);
          columns.append(QVariant(segments));
        }

        return columns;
      }
    }

    return QVariant();
  }

signals:
  void statusFinished(bool visible);

private:
  struct Parent {
    Parent(const git::Commit &commit, const QColor &color, bool tainted = false)
        : commit(commit), color(color), tainted(tainted) {}

    QColor taintedColor(const git::Commit &commit = git::Commit()) const {
      return (tainted && this->commit != commit) ? kTaintedColor : color;
    }

    git::Commit commit;
    QColor color;
    bool tainted;
  };

  struct Segment {
    Segment(GraphSegment segment, QColor color)
        : segment(segment), color(color) {}

    GraphSegment segment;
    QColor color;
  };

  using Column = QList<Segment>;

  struct Row {
    Row(const git::Commit &commit, const QVector<Column> &columns)
        : commit(commit), columns(columns) {}

    git::Commit commit;
    QVector<Column> columns;
  };

  int indexOf(const git::Commit &commit) const {
    int count = mParents.size();
    for (int i = 0; i < count; ++i) {
      if (mParents.at(i).commit == commit)
        return i;
    }

    return -1;
  }

  bool contains(const git::Commit &commit, const QList<Row> &rows) const {
    foreach (const Row &row, mRows) {
      if (row.commit == commit)
        return true;
    }

    foreach (const Row &row, rows) {
      if (row.commit == commit)
        return true;
    }

    return false;
  }

  // The commit and parents parameters represent the current row.
  // The mParents member represents the next row after this one.
  QVector<Column> columns(const git::Commit &commit,
                          const QList<Parent> &parents, bool root) {
    int count = parents.size();
    QVector<Column> columns(count);

    // Add incoming paths.
    int incoming = root ? count - 1 : count;
    for (int i = 0; i < incoming; ++i)
      columns[i] << Segment(Top, parents.at(i).taintedColor());

    // Add outgoing paths.
    for (int i = 0; i < count; ++i) {
      // Get the successors of this column.
      QList<git::Commit> successors;
      const Parent &parent = parents.at(i);
      if (parent.commit == commit) {
        successors = parent.commit.parents();
      } else {
        successors.append(parent.commit);
      }

      // Add a path to each successor.
      foreach (const git::Commit &successor, successors) {
        // Find index of parent in next row.
        int index = indexOf(successor);
        if (index < 0)
          continue;

        // Handle multiple commits that share the same parent.
        bool single = (successors.size() == 1);
        const QColor &color =
            single ? parent.taintedColor(commit) : mParents.at(index).color;

        if (index < i) {
          // out to the left
          columns[index] << Segment(RightIn, color);
          for (int j = index + 1; j < i; ++j)
            columns[j] << Segment(Cross, color);
          columns[i] << Segment(LeftOut, color);

        } else if (index > i) {
          // out to the right
          columns[i] << Segment(RightOut, color);
          for (int j = i + 1; j < index; ++j)
            columns[j] << Segment(Cross, color);
          if (index == columns.size())
            columns.append(Column());
          columns[index] << Segment(LeftIn, color);

        } else { // index == i
          // out the bottom
          columns[index] << Segment(Bottom, color);
        }
      }
    }

    // Add middle section last.
    for (int i = 0; i < count; ++i) {
      const Parent &parent = parents.at(i);
      bool dot = (parent.commit == commit);
      columns[i] << Segment(dot ? Dot : Middle, parent.taintedColor());
    }

    return columns;
  }

  QColor nextColor() {
    // Get the first unused (or least used) color.
    QMap<QString, int> counts;
    foreach (const Parent &parent, mParents)
      counts[parent.color.name()]++;

    int count = 0;
    QList<QColor> colors = Application::theme()->branchTopologyEdges();
    forever {
      foreach (const QColor &color, colors) {
        if (counts.value(color.name()) == count)
          return color;
      }

      ++count;
    }

    Q_UNREACHABLE();
    return QColor();
  }

  QTimer mTimer;
  int mProgress = 0;

  DiffCallbacks mStatusCallbacks;
  QFutureWatcher<git::Diff> mStatus;

  QString mPathspec;
  git::Reference mRef;
  git::RevWalk mWalker;
  git::Repository mRepo;

  QList<Row> mRows;
  QList<Parent> mParents;

  // walker settings
  bool mSuppressResetWalker{false};
  CommitList::RefsFilter mRefsFilter{CommitList::RefsFilter::AllRefs};
  bool mSortDate = true;
  bool mShowCleanStatus = true;
  bool mGraphVisible = true;
};

/*!
 * \brief The ListModel class
 * Used to show a list of commits. This is used when a filter is used
 */
class ListModel : public QAbstractListModel {
public:
  ListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

  void setList(const QList<git::Commit> &commits) {
    beginResetModel();
    mCommits = commits;
    endResetModel();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    return mCommits.size();
  }

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    switch (role) {
      case CommitList::Role::DiffRole: {
        git::Commit commit = mCommits.at(index.row());
        bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
        git::Diff diff = commit.diff(git::Commit(), -1, ignoreWhitespace);
        diff.findSimilar();
        return QVariant::fromValue(diff);
      }

      case CommitList::Role::CommitRole:
        return QVariant::fromValue(mCommits.at(index.row()));
    }

    return QVariant();
  }

private:
  QList<git::Commit> mCommits;
};

class CommitDelegate : public QStyledItemDelegate {
public:
  CommitDelegate(const git::Repository &repo, QObject *parent = nullptr)
      : QStyledItemDelegate(parent), mRepo(repo) {
    updateRefs();

    git::RepositoryNotifier *notifier = repo.notifier();
    connect(notifier, &git::RepositoryNotifier::referenceUpdated, this,
            &CommitDelegate::updateRefs);
    connect(notifier, &git::RepositoryNotifier::referenceAdded, this,
            &CommitDelegate::updateRefs);
    connect(notifier, &git::RepositoryNotifier::referenceRemoved, this,
            &CommitDelegate::updateRefs);
  }

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    bool compact = Settings::instance()
                       ->value(Setting::Id::ShowCommitsInCompactMode)
                       .toBool();
    bool showAuthor = Settings::instance()
                          ->value(Setting::Id::ShowCommitsAuthor, true)
                          .toBool();
    bool showDate = Settings::instance()
                        ->value(Setting::Id::ShowCommitsDate, true)
                        .toBool();
    bool showId =
        Settings::instance()->value(Setting::Id::ShowCommitsId, true).toBool();
    LayoutConstants constants = layoutConstants(compact);

    bool active = (opt.state & QStyle::State_Active);
    bool selected = (opt.state & QStyle::State_Selected);
    auto group = active ? QPalette::Active : QPalette::Inactive;
    auto textRole = selected ? QPalette::HighlightedText : QPalette::Text;
    auto brightRole = selected ? QPalette::WindowText : QPalette::BrightText;
    QPalette palette = Application::theme()->commitList();
    QColor text = palette.color(group, textRole);
    QColor bright = palette.color(group, brightRole);
    QColor highlight = palette.color(group, QPalette::Highlight);

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    // Draw background.
    if (selected) {
      painter->fillRect(opt.rect, highlight);
    }

    // Draw busy indicator.
    if (opt.features & QStyleOptionViewItem::HasDecoration) {
      QRect rect = decorationRect(option, index);
      int progress = index.data(Qt::DecorationRole).toInt();
      ProgressIndicator::paint(painter, rect, bright, progress, opt.widget);
    }

    // Set default foreground color.
    painter->setPen(text);

    // Use default pen color for dot.
    QPen dot = painter->pen();
    dot.setWidth(2);

    // Copy content rect.
    QRect rect = opt.rect;
    rect.setX(rect.x() + 2);

    int totalWidth = rect.width();

    // Draw graph.
    painter->save();
    QVariantList columns = index.data(CommitList::Role::GraphRole).toList();
    QVariantList colorColumns =
        index.data(CommitList::Role::GraphColorRole).toList();
    for (int i = 0; i < columns.size(); ++i) {
      int x = rect.x();
      int y = rect.y();
      int w = opt.fontMetrics.ascent();
      int h = opt.rect.height();
      int h_2 = h / 2;
      int h_4 = h / 4;

      // radius
      int r = w / 3;

      // xs
      int x1 = x + (w / 2);
      int x2 = x + w;

      // ys
      int y1 = y + h_2 - r;
      int y2 = y + h_2;
      int y3 = y + h_2 + r;
      int y4 = y + h_2 + h_4;
      int y5 = y + h;

      QVariantList segments = columns.at(i).toList();
      QVariantList colors = colorColumns.at(i).toList();
      for (int j = 0; j < segments.size(); ++j) {
        QColor color = colors.at(j).value<QColor>();
        QPen pen(color, 2);
        if (color == kTaintedColor) {
          pen.setStyle(Qt::DashLine);
          pen.setDashPattern({2, 2});
        }

        painter->setPen(pen);
        switch (segments.at(j).toInt()) {
          case Dot:
            painter->setPen(dot);
            painter->drawEllipse(QPoint(x1, y2), r, r);
            break;

          case Top:
            painter->drawLine(x1, y, x1, y1);
            break;

          case Middle:
            painter->drawLine(x1, y1, x1, y3);
            break;

          case Bottom:
            painter->drawLine(x1, y3, x1, y5);
            break;

          case Cross:
            painter->drawLine(x, y4, x2, y4);
            break;

          case RightOut: {
            QPainterPath path;
            path.moveTo(x1, y3);
            path.quadTo(x1, y4, x2, y4);
            painter->drawPath(path);
            break;
          }

          case LeftOut: {
            QPainterPath path;
            path.moveTo(x1, y3);
            path.quadTo(x1, y4, x, y4);
            painter->drawPath(path);
            break;
          }

          case RightIn: {
            QPainterPath path;
            path.moveTo(x1, y5);
            path.quadTo(x1, y4, x2, y4);
            painter->drawPath(path);
            break;
          }

          case LeftIn: {
            QPainterPath path;
            path.moveTo(x1, y5);
            path.quadTo(x1, y4, x, y4);
            painter->drawPath(path);
            break;
          }
        }
      }

      rect.setX(x + w);

      // Finish early if the graph exceeds one third of the available space.
      if (rect.x() > opt.rect.width() / 3)
        break;
    }

    painter->restore();

    // Adjust margins.
    rect.setY(rect.y() + constants.vMargin);
    rect.setX(rect.x() + constants.hMargin);

    // Star has enough padding in compact mode.
    if (!compact)
      rect.setWidth(rect.width() - constants.hMargin);

    // Draw content.
    git::Commit commit =
        index.data(CommitList::Role::CommitRole).value<git::Commit>();
    if (!commit.isValid()) {
      // special case for uncommitted changes
      QString message = index.model()->data(index).toString();
      painter->save();
      QFont italic = opt.font;
      italic.setItalic(true);
      painter->setFont(italic);
      painter->drawText(opt.rect, Qt::AlignCenter, message);
      painter->restore();
    } else {
      const QFontMetrics &fm = opt.fontMetrics;
      QRect star = rect;

      QDateTime date = commit.committer().date().toLocalTime();
      QString timestamp =
          (date.date() == QDate::currentDate())
              ? QLocale().toString(date.time(), QLocale::ShortFormat)
              : QLocale().toString(date.date(), QLocale::ShortFormat);
      int timestampWidth = fm.horizontalAdvance(timestamp);

      if (compact) {
        int maxWidthRefs = rect.width() * 0.5; // Max 50%
        const int minWidthRefs = 50;           // At least display the ellipsis
        const int minWidthDesc = 100;
        int minDisplayWidthDate = 350;

        // Star always takes up its height on the right side.
        star.setX(star.x() + star.width() - star.height());
        star.setY(star.y() - constants.vMargin);
        rect.setWidth(rect.width() - star.width());

        // Draw commit id.
        if (showId) {
          QString id = commit.id().toString().left(kShortIdSize);
          int idWidth = maxShortIdWidth(fm);

          QRect commitRect = rect;
          commitRect.setX(commitRect.x() + commitRect.width() - idWidth);
          painter->save();
          painter->drawText(commitRect, Qt::AlignLeft, id);
          painter->restore();
          rect.setWidth(rect.width() - idWidth - constants.hMargin);
        }

        // Draw date. Only if it is not the same as previous?
        if (showDate && rect.width() > minWidthDesc + timestampWidth + 8 &&
            totalWidth > minDisplayWidthDate) {
          painter->save();
          painter->setPen(bright);
          painter->drawText(rect, Qt::AlignRight, timestamp);
          painter->restore();
          rect.setWidth(rect.width() - timestampWidth - constants.hMargin);
        }

        // Draw Name.
        if (showAuthor) {
          QString name = commit.author().name() + "  ";
          painter->save();
          QFont bold = opt.font;
          bold.setBold(true);
          painter->setFont(bold);
          painter->drawText(rect, Qt::AlignRight, name);
          painter->restore();
          const QFontMetrics boldFm(bold);
          rect.setWidth(rect.width() - boldFm.horizontalAdvance(name) -
                        constants.hMargin);
        }

        // Calculate remaining width for the references.
        QRect ref = rect;
        int refsWidth = ref.width() - minWidthDesc;
        if (maxWidthRefs <= minWidthRefs)
          maxWidthRefs = minWidthRefs;
        if (refsWidth < minWidthRefs)
          refsWidth = minWidthRefs;
        if (refsWidth > maxWidthRefs)
          refsWidth = maxWidthRefs;
        ref.setWidth(refsWidth);

        // Draw references.
        int badgesWidth = rect.x();
        QList<Badge::Label> refs = mRefs.value(commit.id());
        if (!refs.isEmpty())
          badgesWidth = Badge::paint(painter, refs, ref, &opt, Qt::AlignLeft);
        rect.setX(badgesWidth); // Comes right after the badges

        // Draw message.
        painter->save();
        painter->setPen(bright);
        QString msg = commit.summary(git::Commit::SubstituteEmoji);
        QString elidedText = fm.elidedText(msg, Qt::ElideRight, rect.width());
        painter->drawText(rect, Qt::ElideRight, elidedText);
        painter->restore();

      } else {

        // Draw Name.
        QString name = "";
        if (showAuthor) {
          name = commit.author().name();
          painter->save();
          QFont bold = opt.font;
          bold.setBold(true);
          painter->setFont(bold);
          painter->drawText(rect, Qt::AlignLeft, name);
          painter->restore();
        }

        // Draw date.
        if (showDate &&
            rect.width() > fm.horizontalAdvance(name) + timestampWidth + 8) {
          painter->save();
          painter->setPen(bright);
          if (showAuthor) {
            painter->drawText(rect, Qt::AlignRight, timestamp);
          } else {
            painter->drawText(rect, Qt::AlignLeft, timestamp);
          }
          painter->restore();
        }

        // Draw id.
        QString id = "";
        if (showId) {
          QRect idRect = rect;
          if (showAuthor || showDate) {
            idRect.setY(idRect.y() + constants.lineSpacing + constants.vMargin);
          }
          id = commit.shortId();
          painter->save();
          painter->drawText(idRect, Qt::AlignLeft, id);
          painter->restore();
        }

        // Draw references.
        QList<Badge::Label> refs = mRefs.value(commit.id());
        if (!refs.isEmpty()) {
          QRect refsRect = rect;
          QString leftText = "";

          if (showDate && showAuthor) {
            refsRect.setY(refsRect.y() + constants.lineSpacing +
                          constants.vMargin);
            if (showId) {
              leftText = id;
            }
          } else {
            if (showDate) {
              leftText = timestamp;
            } else if (showAuthor) {
              leftText = name;
            } else if (showId) {
              leftText = id;
            }
          }
          refsRect.setX(refsRect.x() + fm.boundingRect(leftText).width() + 6);
          Badge::paint(painter, refs, refsRect, &opt);
        }

        int numOptional = 0;
        if (showId)
          ++numOptional;
        if (showAuthor)
          ++numOptional;
        if (showDate)
          ++numOptional;
        if (numOptional > 1) {
          rect.setY(rect.y() + constants.lineSpacing + constants.vMargin);
        }

        rect.setY(rect.y() + constants.lineSpacing + constants.vMargin);

        // Divide remaining rectangle.
        star = rect;
        star.setX(star.x() + star.width() - star.height());
        QRect text = rect;
        text.setWidth(text.width() - star.width());

        // Draw message.
        painter->save();
        painter->setPen(bright);
        QString msg = commit.summary(git::Commit::SubstituteEmoji);
        QTextLayout layout(msg, painter->font());
        layout.beginLayout();

        QTextLine line = layout.createLine();
        if (line.isValid()) {
          int width = text.width();
          line.setLineWidth(width);
          int len = line.textLength();
          painter->drawText(text, Qt::AlignLeft, msg.left(len));

          if (len < msg.length()) {
            text.setY(text.y() + constants.lineSpacing);
            QString elided = fm.elidedText(msg.mid(len), Qt::ElideRight, width);
            painter->drawText(text, Qt::AlignLeft, elided);
          }
        }

        layout.endLayout();
        painter->restore();
      }

      // Draw star.
      bool starred = commit.isStarred();
      const QAbstractItemView *view =
          static_cast<const QAbstractItemView *>(opt.widget);
      QPoint pos = view->viewport()->mapFromGlobal(QCursor::pos());
      if (starred || (view->underMouse() && view->indexAt(pos) == index)) {
        painter->save();

        // Calculate outer radius and vertices.
        qreal r = (star.height() / 2.0) - constants.starPadding;
        qreal x = star.x() + (star.width() / 2.0);
        qreal y = star.y() + (star.height() / 2.0);
        qreal x1 = r * qCos(M_PI / 10.0);
        qreal y1 = -r * qSin(M_PI / 10.0);
        qreal x2 = r * qCos(17.0 * M_PI / 10.0);
        qreal y2 = -r * qSin(17.0 * M_PI / 10.0);

        // Calculate inner radius and vertices.
        qreal xi = ((y1 + r) * x2) / (y2 + r);
        qreal ri = qSqrt(qPow(xi, 2.0) + qPow(y1, 2.0));
        qreal xi1 = ri * qCos(3.0 * M_PI / 10.0);
        qreal yi1 = -ri * qSin(3.0 * M_PI / 10.0);
        qreal xi2 = ri * qCos(19.0 * M_PI / 10.0);
        qreal yi2 = -ri * qSin(19.0 * M_PI / 10.0);

        QPolygonF polygon({QPointF(0, -r), QPointF(xi1, yi1), QPointF(x1, y1),
                           QPointF(xi2, yi2), QPointF(x2, y2), QPointF(0, ri),
                           QPointF(-x2, y2), QPointF(-xi2, yi2),
                           QPointF(-x1, y1), QPointF(-xi1, yi1)});

        if (starred)
          painter->setBrush(Application::theme()->star());

        painter->setPen(QPen(bright, 1.25));
        painter->drawPolygon(polygon.translated(x, y));
        painter->restore();
      }
    }

    // Is the next index selected?
    bool nextSelected = false;

#ifndef Q_OS_WIN
    // Draw separator between selected indexes.
    QModelIndex next = index.sibling(index.row() + 1, 0);
    if (next.isValid()) {
      const QAbstractItemView *view =
          static_cast<const QAbstractItemView *>(opt.widget);
      nextSelected = view->selectionModel()->isSelected(next);
    }
#endif

    // Draw separator line.
    if (!compact && selected == nextSelected) {
      painter->save();
      painter->setRenderHints(QPainter::Antialiasing, false);
      painter->setPen(selected ? text : opt.palette.color(QPalette::Dark));
      painter->drawLine(rect.bottomLeft(), rect.bottomRight());
      painter->restore();
    }

    painter->restore();
  }

  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override {
    bool compact = Settings::instance()
                       ->value(Setting::Id::ShowCommitsInCompactMode)
                       .toBool();
    LayoutConstants constants = layoutConstants(compact);

    int lineHeight = constants.lineSpacing + constants.vMargin;
    return QSize(0, lineHeight * (compact ? 1 : 4));
  }

  QRect decorationRect(const QStyleOptionViewItem &option,
                       const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    QStyle::SubElement se = QStyle::SE_ItemViewItemDecoration;
    return style->subElementRect(se, &opt, opt.widget);
  }

  QRect starRect(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const {
    bool compact = Settings::instance()
                       ->value(Setting::Id::ShowCommitsInCompactMode)
                       .toBool();
    LayoutConstants constants = layoutConstants(compact);

    QRect rect = option.rect;
    int length = constants.lineSpacing * 2;
    rect.setX(rect.x() + rect.width() - length);
    rect.setY(rect.y() + rect.height() - length);
    rect.setWidth(rect.width() - constants.starPadding);
    rect.setHeight(rect.height() - constants.starPadding);
    return rect;
  }

protected:
  void initStyleOption(QStyleOptionViewItem *option,
                       const QModelIndex &index) const override {
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.data(Qt::DecorationRole).canConvert<int>())
      option->decorationSize = ProgressIndicator::size();
  }

private:
  struct LayoutConstants {
    const int starPadding;
    const int lineSpacing;
    const int vMargin;
    const int hMargin;
  };

  LayoutConstants layoutConstants(bool compact) const {
    return {compact ? 7 : 8, compact ? 23 : 16, compact ? 5 : 2, 4};
  }

  void updateRefs() {
    mRefs.clear();

    if (mRepo.isHeadDetached()) {
      git::Reference head = mRepo.head();
      mRefs[head.target().id()].append(
          {Badge::Label::Type::Ref, head.name(), true});
    }

    foreach (const git::Reference &ref, mRepo.refs()) {
      if (git::Commit target = ref.target())
        mRefs[target.id()].append(
            {Badge::Label::Type::Ref, ref.name(), ref.isHead(), ref.isTag()});
    }
  }

  int maxShortIdWidth(const QFontMetrics &fm) const {
    if (mMaxShortIdWidth < 0) {
      for (char ch = 'a'; ch <= 'f'; ++ch) {
        int width = fm.boundingRect(QString(kShortIdSize, ch)).width();
        mMaxShortIdWidth = qMax(mMaxShortIdWidth, width);
      }

      for (char ch = '0'; ch <= '9'; ++ch) {
        int width = fm.boundingRect(QString(kShortIdSize, ch)).width();
        mMaxShortIdWidth = qMax(mMaxShortIdWidth, width);
      }
    }

    return mMaxShortIdWidth;
  }

  git::Repository mRepo;
  QMap<git::Id, QList<Badge::Label>> mRefs;

  mutable int mMaxShortIdWidth = -1;
};

class SelectionModel : public QItemSelectionModel {
public:
  SelectionModel(QAbstractItemModel *model) : QItemSelectionModel(model) {}

  void select(const QItemSelection &selection,
              QItemSelectionModel::SelectionFlags command) {
    if ((command == QItemSelectionModel::Select ||
         command == QItemSelectionModel::SelectCurrent ||
         command == (QItemSelectionModel::Current |
                     QItemSelectionModel::ClearAndSelect)) &&
        (selectedIndexes().size() >= 2 || selection.indexes().size() > 1))
      return;

    QItemSelectionModel::select(selection, command);
  }
};

} // namespace

static Hotkey selectCommitDownHotKey = HotkeyManager::registerHotkey(
    "j", "commitList/selectCommitDown", "CommitList/Select Next Commit Down");

static Hotkey selectCommitUpHotKey = HotkeyManager::registerHotkey(
    "k", "commitList/selectCommitUp", "CommitList/Select Next Commit Up");

CommitList::CommitList(Index *index, QWidget *parent)
    : QListView(parent), mIndex(index) {
  Theme *theme = Application::theme();
  setPalette(theme->commitList());

  git::Repository repo = index->repo();
  mList = new ListModel(this);
  mModel = new CommitModel(repo, this);

  setMouseTracking(true);
  setUniformItemSizes(true);
  setAttribute(Qt::WA_MacShowFocusRect, false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  setModel(mModel);
  setItemDelegate(new CommitDelegate(repo, this));

  connect(mModel, &QAbstractItemModel::modelAboutToBeReset, this,
          &CommitList::storeSelection);
  connect(mModel, &QAbstractItemModel::modelReset, this,
          &CommitList::restoreSelection);
  connect(mList, &QAbstractItemModel::modelAboutToBeReset, this,
          &CommitList::storeSelection);
  connect(mList, &QAbstractItemModel::modelReset, this,
          &CommitList::restoreSelection);

  CommitModel *model = static_cast<CommitModel *>(mModel);
  connect(model, &CommitModel::statusFinished, [this, model](bool visible) {
    mRestoreSelection = true; // Reset to default
    // Fake a selection notification if the diff is visible and selected.
    // FIXME: Should we reference `model` or `this->mModel` here?
    if (visible && selectionModel()->isSelected(mModel->index(0, 0)))
      resetSelection();

    // Select the first commit if the selection was cleared.
    if (selectedIndexes().isEmpty())
      selectFirstCommit();

    // Notify main window.
    emit statusChanged(visible);
  });

  git::RepositoryNotifier *notifier = repo.notifier();
  connect(notifier, &git::RepositoryNotifier::referenceUpdated,
          [this](const git::Reference &ref, bool restoreSelection) {
            mRestoreSelection = restoreSelection;
            resetReference(ref);
          });
  connect(notifier, &git::RepositoryNotifier::workdirChanged, [this] {
    resetReference(static_cast<const CommitModel *>(mModel)->reference());
  });

  connect(this, &CommitList::entered,
          [this](const QModelIndex &index) { update(index); });

  QShortcut *shortcut = new QShortcut(this);
  selectCommitDownHotKey.use(shortcut);
  connect(shortcut, &QShortcut::activated, [this] { selectCommitRelative(1); });

  shortcut = new QShortcut(this);
  selectCommitUpHotKey.use(shortcut);
  connect(shortcut, &QShortcut::activated,
          [this] { selectCommitRelative(-1); });

#ifdef Q_OS_MAC
  QFont font = this->font();
  font.setPointSize(13);
  setFont(font);
#endif
}

git::Diff CommitList::status() const {
  return static_cast<CommitModel *>(mModel)->status();
}

QString CommitList::selectedRange() const {
  QList<git::Commit> commits = selectedCommits();
  if (commits.isEmpty())
    return !selectedIndexes().isEmpty() ? "status" : QString();

  git::Commit first = commits.first();
  if (commits.size() == 1)
    return first.id().toString();

  git::Commit last = commits.last();
  return QString("%1..%2").arg(last.id().toString(), first.id().toString());
}

git::Diff CommitList::selectedDiff() const {
  QModelIndexList indexes = sortedIndexes();
  DebugRefresh("Selected indices count: " << indexes.count());
  for (const auto &index : indexes) {
    const auto &id = index.data(CommitRole).value<git::Commit>().shortId();
    (void)id; // Unused in release builds
    DebugRefresh("Commit: " << id);
  }
  if (indexes.isEmpty())
    return git::Diff();

  if (indexes.size() == 1)
    return indexes.first().data(DiffRole).value<git::Diff>();

  git::Commit first = indexes.first().data(CommitRole).value<git::Commit>();
  if (!first.isValid())
    return git::Diff();

  git::Commit last = indexes.last().data(CommitRole).value<git::Commit>();
  bool ignoreWhitespace = Settings::instance()->isWhitespaceIgnored();
  git::Diff diff = first.diff(last, -1, ignoreWhitespace);
  diff.findSimilar();
  return diff;
}

QList<git::Commit> CommitList::selectedCommits() const {
  QList<git::Commit> selectedCommits;
  foreach (const QModelIndex &index, sortedIndexes()) {
    git::Commit commit = index.data(CommitRole).value<git::Commit>();
    if (commit.isValid())
      selectedCommits.append(commit);
  }

  return selectedCommits;
}

void CommitList::cancelStatus() {
  static_cast<CommitModel *>(mModel)->cancelStatus();
}

void CommitList::setReference(const git::Reference &ref) {
  static_cast<CommitModel *>(mModel)->setReference(ref);
  if (!isResetWalkerSuppressed())
    updateModel();
  setFocus();
}

void CommitList::setFilter(const QString &filter) {
  mFilter = filter.simplified();
  updateModel();
}

void CommitList::setPathspec(const QString &pathspec, bool index) {
  if (index) {
    setFilter(!pathspec.isEmpty() ? kPathspecFmt.arg(pathspec) : QString());
  } else {
    static_cast<CommitModel *>(mModel)->setPathspec(pathspec);
  }
}

void CommitList::setCommits(const QList<git::Commit> &commits) {
  setModel(mList);
  static_cast<ListModel *>(mList)->setList(commits);
}

void CommitList::selectReference(const git::Reference &ref) {
  if (!ref.isValid())
    return;

  QModelIndex index = model()->index(0, 0);
  if (ref.isHead() && !index.data(CommitRole).isValid()) {
    selectFirstCommit();
  } else {
    selectRange(ref.target().id().toString());
  }
}

void CommitList::resetSelection(bool spontaneous) {
  // Just notify.
  mSpontaneous = spontaneous;
  notifySelectionChanged();
  mSpontaneous = true;
}

void CommitList::selectFirstCommit(bool spontaneous) {
  QModelIndex index = model()->index(0, 0);
  const auto commit = index.data(CommitRole).value<git::Commit>();
  if (commit.isValid())
    DebugRefresh("Commit id: " << commit.shortId());
  else
    DebugRefresh("Invalid commit");
  if (index.isValid()) {
    selectIndexes(QItemSelection(index, index), QString(), spontaneous);
  } else {
    emit diffSelected(git::Diff());
  }
}

void CommitList::selectCommitRelative(int offset) {
  QModelIndexList indices = selectionModel()->selectedIndexes();
  QModelIndex index = indices[0];
  if (!index.isValid()) {
    return;
  }
  QModelIndex new_index = model()->index(index.row() + offset, index.column());
  if (!new_index.isValid()) {
    return;
  }
  selectIndexes(QItemSelection(new_index, new_index), QString(), true);
}

bool CommitList::selectRange(const QString &range, const QString &file,
                             bool spontaneous) {
  // Try to select the "status" index.
  QModelIndex index = model()->index(0, 0);
  if (range == "status" && !index.data(CommitRole).isValid()) {
    selectFirstCommit();
    return true;
  }

  QStringList ids = range.split("..");
  if (ids.size() > 2)
    return false;

  // Invert range.
  bool one = (ids.size() == 1);
  git::Repository repo = RepoView::parentView(this)->repo();
  git::Commit firstCommit = repo.lookupCommit(ids.last());
  git::Commit lastCommit = one ? firstCommit : repo.lookupCommit(ids.first());

  // Check for already selected range.
  QModelIndexList indexes = sortedIndexes();
  if (indexes.size() >= 2) {
    git::Commit first = indexes.first().data(CommitRole).value<git::Commit>();
    git::Commit last = indexes.last().data(CommitRole).value<git::Commit>();
    if (first.isValid() && first == firstCommit && last.isValid() &&
        last == lastCommit)
      return false;
  }

  // Find indexes.
  QItemSelection selection;
  QModelIndex first = findCommit(firstCommit);
  if (!first.isValid())
    return false;
  selection.select(first, first);

  if (lastCommit != firstCommit) {
    QModelIndex last = findCommit(lastCommit);
    if (!last.isValid())
      return false;
    selection.select(last, last);
  }

  selectIndexes(selection, file, spontaneous);
  return true;
}

void CommitList::suppressResetWalker(bool suppress) {
  static_cast<CommitModel *>(mModel)->suppressResetWalker(suppress);
}

void CommitList::resetReference(const git::Reference &ref) {
  static_cast<CommitModel *>(mModel)->resetReference(ref);
}

bool CommitList::isResetWalkerSuppressed() {
  return static_cast<CommitModel *>(mModel)->isResetWalkerSuppressed();
}

void CommitList::resetSettings() {
  static_cast<CommitModel *>(mModel)->resetSettings(true);
}

void CommitList::setModel(QAbstractItemModel *model) {
  if (model == this->model())
    return;

  storeSelection();

  // Destroy the previous selection model.
  delete selectionModel();

  QListView::setModel(model);

  // Destroy the selection model created by Qt.
  delete selectionModel();

  SelectionModel *selectionModel = new SelectionModel(model);
  connect(
      selectionModel, &QItemSelectionModel::selectionChanged,
      [this](const QItemSelection &selected, const QItemSelection &deselected) {
        // Update the index before each selected/deselected range.
        foreach (const QItemSelectionRange &range, selected + deselected) {
          if (int row = range.top())
            update(this->model()->index(row - 1, 0));
        }

        notifySelectionChanged();
      });

  setSelectionModel(selectionModel);

  restoreSelection();
}

/// @brief Helper function to add a list of items to a menu.
/// A single item is added directly to the menu, whereas multiple items will
/// be added to a sub-menu.
static void addMenuEntries(QMenu &menu, const QString &operation,
                           const QList<git::Reference> &items,
                           std::function<void(const git::Reference &)> action) {
  QMenu *submenu = &menu;
  QString entryName(operation + " %1");
  if (items.count() > 1) {
    submenu = menu.addMenu(operation);
    entryName = QString("%1");
  }
  for (const git::Reference &ref : items) {
    submenu->addAction(entryName.arg(ref.name()),
                       [action, ref] { action(ref); });
  }
}

void CommitList::contextMenuEvent(QContextMenuEvent *event) {
  QModelIndex index = indexAt(event->pos());
  if (!index.isValid())
    return;

  RepoView *view = RepoView::parentView(this);
  git::Commit commit = index.data(CommitRole).value<git::Commit>();

  if (!commit.isValid()) {
    QMenu menu;

    // clean
    QStringList untracked;
    if (git::Diff diff = status()) {
      for (int i = 0; i < diff.count(); i++) {
        if (diff.status(i) == GIT_DELTA_UNTRACKED)
          untracked.append(diff.name(i));
      }
    }

    QAction *clean =
        menu.addAction(tr("Remove Untracked Files"),
                       [view, untracked] { view->clean(untracked); });

    clean->setEnabled(!untracked.isEmpty());

    menu.exec(event->globalPos());
    return;
  }

  QMenu menu;
  menu.setToolTipsVisible(true);

  // stash
  git::Reference ref = static_cast<CommitModel *>(mModel)->reference();
  if (ref.isValid() && ref.isStash()) {
    menu.addAction(tr("Apply"),
                   [view, index] { view->applyStash(index.row()); });

    menu.addAction(tr("Pop"), [view, index] { view->popStash(index.row()); });

    menu.addAction(tr("Drop"), [view, index] { view->dropStash(index.row()); });

  } else {
    // multiple selection
    bool anyStarred = false;
    foreach (const QModelIndex &index, selectionModel()->selectedIndexes()) {
      if (index.data(CommitRole).isValid() &&
          index.data(CommitRole).value<git::Commit>().isStarred()) {
        anyStarred = true;
        break;
      }
    }

    menu.addAction(anyStarred ? tr("Unstar") : tr("Star"), [this, anyStarred] {
      foreach (const QModelIndex &index, selectionModel()->selectedIndexes())
        if (index.data(CommitRole).isValid())
          index.data(CommitRole).value<git::Commit>().setStarred(!anyStarred);
    });

    // single selection
    if (selectionModel()->selectedIndexes().size() <= 1) {
      menu.addSeparator();

      menu.addAction(tr("Add Tag..."),
                     [view, commit] { view->promptToAddTag(commit); });

      menu.addAction(tr("New Branch..."),
                     [view, commit] { view->promptToCreateBranch(commit); });

      // Add operations on existing references; there may be 0, 1, or multiple
      // of each type of reference on a commit.
      QList<git::Reference> rename_branches;
      QList<git::Reference> tags;
      QList<git::Reference> delete_branches;
      QList<git::Reference> all_branches; // used later
      for (const git::Reference &ref : commit.refs()) {
        if (ref.isTag()) {
          tags.append(ref);
        } else if (ref.isBranch()) {
          all_branches.append(ref);
          if (ref.isLocalBranch()) {
            rename_branches.append(ref);
            if (view->repo().head().name() != ref.name()) {
              delete_branches.append(ref);
            }
          }
        }
      }

      if (rename_branches.count() > 0 || delete_branches.count() > 0 ||
          tags.count() > 0) {
        menu.addSeparator();
      }
      addMenuEntries(menu, tr("Rename Branch"), rename_branches,
                     std::bind(&RepoView::promptToRenameBranch, view,
                               std::placeholders::_1));

      addMenuEntries(menu, tr("Delete Branch"), delete_branches,
                     std::bind(&RepoView::promptToDeleteBranch, view,
                               std::placeholders::_1));

      addMenuEntries(
          menu, tr("Delete Tag"), tags,
          std::bind(&RepoView::promptToDeleteTag, view, std::placeholders::_1));
      menu.addSeparator();

      menu.addAction(tr("Merge..."), [view, commit] {
        MergeDialog *dialog =
            new MergeDialog(RepoView::Merge, view->repo(), view);
        connect(dialog, &QDialog::accepted, [view, dialog] {
          git::AnnotatedCommit upstream;
          git::Reference ref = dialog->reference();
          if (!ref.isValid())
            upstream = dialog->target().annotatedCommit();
          view->merge(dialog->flags(), ref, upstream);
        });

        dialog->setCommit(commit);
        dialog->open();
      });

      menu.addAction(tr("Rebase..."), [view, commit] {
        MergeDialog *dialog =
            new MergeDialog(RepoView::Rebase, view->repo(), view);
        connect(dialog, &QDialog::accepted, [view, dialog] {
          git::AnnotatedCommit upstream;
          git::Reference ref = dialog->reference();
          if (!ref.isValid())
            upstream = dialog->target().annotatedCommit();
          view->merge(dialog->flags(), ref, upstream);
        });

        dialog->setCommit(commit);
        dialog->open();
      });

      menu.addAction(tr("Squash..."), [view, commit] {
        MergeDialog *dialog =
            new MergeDialog(RepoView::Squash, view->repo(), view);
        connect(dialog, &QDialog::accepted, [view, dialog] {
          git::AnnotatedCommit upstream;
          git::Reference ref = dialog->reference();
          if (!ref.isValid())
            upstream = dialog->target().annotatedCommit();
          view->merge(dialog->flags(), ref, upstream);
        });

        dialog->setCommit(commit);
        dialog->open();
      });

      menu.addSeparator();

      menu.addAction(tr("Revert"), [view, commit] { view->revert(commit); });

      menu.addAction(tr("Cherry-pick"),
                     [view, commit] { view->cherryPick(commit); });

      menu.addSeparator();

      git::Reference head = view->repo().head();
      auto submenu = &menu;
      auto entryName = tr("Checkout %1");
      if (all_branches.count() > 1) {
        submenu = menu.addMenu(tr("Checkout"));
        entryName = QString("%1");
      }
      for (const git::Reference &ref : all_branches) {
        if (ref.isLocalBranch()) {
          QAction *checkout = submenu->addAction(
              entryName.arg(ref.name()), [view, ref] { view->checkout(ref); });

          checkout->setEnabled(head.isValid() &&
                               head.qualifiedName() != ref.qualifiedName() &&
                               !view->repo().isBare());
        } else if (ref.isRemoteBranch()) {
          QAction *checkout = submenu->addAction(
              entryName.arg(ref.name()), [view, ref] { view->checkout(ref); });

          // Calculate local branch name in the same way as checkout() does
          QString local = ref.name().section('/', 1);
          if (!head.isValid()) { // I'm not sure when this can happen
            checkout->setEnabled(false);
          } else if (head.name() == local) {
            checkout->setEnabled(false);
            checkout->setToolTip(tr("Local branch is already checked out"));
          } else if (view->repo().isBare()) {
            checkout->setEnabled(false);
            checkout->setToolTip(tr("This is a bare repository"));
          }
        }
      }

      QString name = commit.detachedHeadName();
      QAction *checkout =
          menu.addAction(tr("Checkout %1").arg(name),
                         [view, commit] { view->checkout(commit); });

      checkout->setEnabled(head.isValid() && head.target() != commit &&
                           !view->repo().isBare());

      menu.addSeparator();

      QMenu *reset = menu.addMenu(tr("Reset"));
      reset->addAction(tr("Soft"))->setData(GIT_RESET_SOFT);
      reset->addAction(tr("Mixed"))->setData(GIT_RESET_MIXED);
      reset->addAction(tr("Hard"))->setData(GIT_RESET_HARD);
      connect(reset, &QMenu::triggered, [view, commit](QAction *action) {
        git_reset_t type = static_cast<git_reset_t>(action->data().toInt());
        view->promptToReset(commit, type);
      });

      reset->setEnabled(head.isValid() && head.isLocalBranch());
    }
  }

  menu.exec(event->globalPos());
}

void CommitList::mouseMoveEvent(QMouseEvent *event) {
  if (mStar.isValid() || mCancel.isValid())
    return;

  QListView::mouseMoveEvent(event);
}

void CommitList::mousePressEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);
  mStar = isStar(index, pos) ? index : QModelIndex();
  mCancel = isDecoration(index, pos) ? index : QModelIndex();

  if (mStar.isValid() || mCancel.isValid())
    return;

  DebugRefresh("time: " << QDateTime::currentDateTime());

  QListView::mousePressEvent(event);
}

void CommitList::mouseReleaseEvent(QMouseEvent *event) {
  QPoint pos = event->pos();
  QModelIndex index = indexAt(pos);
  if (mStar == index && isStar(index, pos)) {
    if (git::Commit commit = index.data(CommitRole).value<git::Commit>()) {
      commit.setStarred(!commit.isStarred());
      update(index); // FIXME: Add signal?
    }
  } else if (mCancel == index && isDecoration(index, pos)) {
    static_cast<CommitModel *>(model())->cancelStatus();
  }

  mStar = QModelIndex();
  mCancel = QModelIndex();

  QListView::mouseReleaseEvent(event);
}

void CommitList::leaveEvent(QEvent *event) {
  viewport()->update();
  QListView::leaveEvent(event);
}

void CommitList::storeSelection() {
  mSelectedRange = selectedRange();
  DebugRefresh("Selected Range: " << mSelectedRange);
  Debug(mSelectedRange);
}

void CommitList::restoreSelection() {
  // Restore selection.
  DebugRefresh(mSelectedRange);
  if (!mRestoreSelection ||
      (!mSelectedRange.isEmpty() && !selectRange(mSelectedRange))) {
    DebugRefresh("Failed to restore");
    emit diffSelected(git::Diff());
  }

  mSelectedRange = QString();
}

void CommitList::updateModel() {
  if (!mFilter.isEmpty()) {
    setCommits(mIndex->commits(mFilter));
    return;
  }

  git::Reference ref = static_cast<CommitModel *>(mModel)->reference();
  if (ref.isValid() && ref.isStash()) {
    setCommits(ref.repo().stashes());
    return;
  }

  // Reset model.
  setModel(mModel);
}

QModelIndexList CommitList::sortedIndexes() const {
  QModelIndexList indexes = selectedIndexes();
  std::sort(indexes.begin(), indexes.end(),
            [](const QModelIndex &lhs, const QModelIndex &rhs) {
              return lhs.row() < rhs.row();
            });

  return indexes;
}

QModelIndex CommitList::findCommit(const git::Commit &commit) {
  // Get the 'uncommitted changes' index.
  QAbstractItemModel *model = this->model();
  if (!commit.isValid()) {
    QModelIndex index = model->index(0, 0);
    git::Commit tmp = index.data(CommitRole).value<git::Commit>();
    return !tmp.isValid() ? index : QModelIndex();
  }

  // Find the id.
  QDateTime date = commit.committer().date();
  for (int i = 0; i < model->rowCount(); ++i) {
    QModelIndex index = model->index(i, 0);
    if (git::Commit tmp = index.data(CommitRole).value<git::Commit>()) {
      if (tmp == commit)
        return index;

      // Cut off search if we find an older commit.
      if (tmp.committer().date() < date)
        return QModelIndex();
    }

    // Load more commits.
    if (i == model->rowCount() - 1 && model->canFetchMore(QModelIndex()))
      model->fetchMore(QModelIndex());
  }

  return QModelIndex();
}

void CommitList::selectIndexes(const QItemSelection &selection,
                               const QString &file, bool spontaneous) {
  mFile = file;
  mSpontaneous = spontaneous;
  selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
  mSpontaneous = true;
  mFile = QString();

  QModelIndexList indexes = selection.indexes();
  if (!indexes.isEmpty())
    scrollTo(indexes.first());
}

void CommitList::notifySelectionChanged() {
  // Multiple selection means that the selected parameter
  // could be empty when there are still indexes selected.
  QModelIndexList indexes = selectedIndexes();
  if (indexes.isEmpty())
    return;

  // Redraw all selected indexes. Separators may have changed.
  foreach (const QModelIndex &index, indexes)
    update(index);
  git::Diff diff = selectedDiff();
  emit diffSelected(diff, mFile, mSpontaneous);
}

bool CommitList::isDecoration(const QModelIndex &index, const QPoint &pos) {
  if (!index.isValid())
    return false;

  CommitDelegate *delegate = static_cast<CommitDelegate *>(itemDelegate());
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);
  return delegate->decorationRect(options, index).contains(pos);
}

bool CommitList::isStar(const QModelIndex &index, const QPoint &pos) {
  if (!index.isValid() || !index.data(CommitRole).isValid())
    return false;

  CommitDelegate *delegate = static_cast<CommitDelegate *>(itemDelegate());
  QStyleOptionViewItem options = viewOptions();
  options.rect = visualRect(index);
  return delegate->starRect(options, index).contains(pos);
}

#include "CommitList.moc"
