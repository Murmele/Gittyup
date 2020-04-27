//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DetailView.h"
#include "Badge.h"
#include "DiffWidget.h"
#include "MenuBar.h"
#include "ContextMenuButton.h"
#include "TreeWidget.h"
#include "TemplateButton.h"
#include "DoubleTreeWidget.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Diff.h"
#include "git/Index.h"
#include "git/Repository.h"
#include "git/Signature.h"
#include "git/TagRef.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyle>
#include <QTextEdit>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWindow>
#include <QtConcurrent>

namespace {

const int kSize = 64;
const char *kCacheKey = "cache_key";
const QString kRangeFmt = "%1..%2";
const QString kDateRangeFmt = "%1-%2";
const QString kBoldFmt = "<b>%1</b>";
const QString kItalicFmt = "<i>%1</i>";
const QString kLinkFmt = "<a href='%1'>%2</a>";
const QString kAuthorFmt = "<b>%1 &lt;%2&gt;</b>";
const QString kAltFmt = "<span style='color: %1'>%2</span>";
const QString kUrl = "http://www.gravatar.com/avatar/%1?s=%2&d=mm";

const Qt::TextInteractionFlags kTextFlags =
  Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;

QString brightText(const QString &text)
{
  return kAltFmt.arg(QPalette().color(QPalette::BrightText).name(), text);
}

class MessageLabel : public QTextEdit
{
public:
  MessageLabel(QWidget *parent = nullptr)
    : QTextEdit(parent)
  {
    setObjectName("MessageLabel");
    setFrameShape(QFrame::NoFrame);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setReadOnly(true);
    document()->setDocumentMargin(0);

    // Notify the layout system when size hint changes.
    connect(document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &MessageLabel::updateGeometry);
  }

protected:
  QSize minimumSizeHint() const override
  {
    QSize size = QTextEdit::minimumSizeHint();
    return QSize(size.width(), fontMetrics().lineSpacing());
  }

  QSize viewportSizeHint() const override
  {
    // Choose the smaller of the height of the document or five lines.
    QSize size = QTextEdit::viewportSizeHint();
    int height = document()->documentLayout()->documentSize().height();
    return QSize(size.width(), qMin(height, 5 * fontMetrics().lineSpacing()));
  }
};

class StackedWidget : public QStackedWidget
{
public:
  StackedWidget(QWidget *parent = nullptr)
    : QStackedWidget(parent)
  {}

  QSize sizeHint() const override
  {
    return currentWidget()->sizeHint();
  }

  QSize minimumSizeHint() const override
  {
    return currentWidget()->minimumSizeHint();
  }
};

class AuthorDate : public QWidget
{
public:
  AuthorDate(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    mAuthor = new QLabel(this);
    mAuthor->setTextInteractionFlags(kTextFlags);

    mDate = new QLabel(this);
    mDate->setTextInteractionFlags(kTextFlags);

    mSpacing = style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing);
  }

  void moveEvent(QMoveEvent *event) override
  {
    updateLayout();
  }

  void resizeEvent(QResizeEvent *event) override
  {
    updateLayout();
  }

  QSize sizeHint() const override
  {
    QSize date = mDate->sizeHint();
    QSize author = mAuthor->sizeHint();
    int width = author.width() + date.width() + mSpacing;
    return QSize(width, qMax(author.height(), date.height()));
  }

  QSize minimumSizeHint() const override
  {
    QSize date = mDate->minimumSizeHint();
    QSize author = mAuthor->minimumSizeHint();
    int width = qMax(author.width(), date.width());
    return QSize(width, qMax(author.height(), date.height()));
  }

  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int width) const override
  {
    int date = mDate->sizeHint().height();
    int author = mAuthor->sizeHint().height();
    bool wrapped = (width < sizeHint().width());
    return wrapped ? (author + date + mSpacing) : qMax(author, date);
  }

  void setAuthor(const QString &author)
  {
    mAuthor->setText(author);
    mAuthor->adjustSize();
    updateGeometry();
  }

  void setDate(const QString &date)
  {
    mDate->setText(date);
    mDate->adjustSize();
    updateGeometry();
  }

private:
  void updateLayout()
  {
    mAuthor->move(0, 0);

    bool wrapped = (width() < sizeHint().width());
    int x = wrapped ? 0 : width() - mDate->width();
    int y = wrapped ? mAuthor->height() + mSpacing : 0;
    mDate->move(x, y);
  }

  QLabel *mAuthor;
  QLabel *mDate;

  int mSpacing;
};

class CommitDetail : public QFrame
{
  Q_OBJECT

public:
  CommitDetail(QWidget *parent = nullptr)
    : QFrame(parent)
  {
    mAuthorDate = new AuthorDate(this);

    mHash = new QLabel(this);
    mHash->setTextInteractionFlags(kTextFlags);

    QToolButton *copy = new QToolButton(this);
    copy->setText(tr("Copy"));
    connect(copy, &QToolButton::clicked, [this] {
      QApplication::clipboard()->setText(mId);
    });

    mRefs = new Badge(QList<Badge::Label>(), this);

    QHBoxLayout *line2 = new QHBoxLayout;
    line2->addWidget(mHash);
    line2->addWidget(copy);
    line2->addStretch();
    line2->addWidget(mRefs);

    mParents = new QLabel(this);
    mParents->setTextInteractionFlags(kTextFlags);

    QHBoxLayout *line3 = new QHBoxLayout;
    line3->addWidget(mParents);
    line3->addStretch();

    QVBoxLayout *details = new QVBoxLayout;
    details->setSpacing(6);
    details->addWidget(mAuthorDate);
    details->addLayout(line2);
    details->addLayout(line3);
    details->addStretch();

    mPicture = new QLabel(this);

    QHBoxLayout *header = new QHBoxLayout;
    header->addLayout(details);
    header->addWidget(mPicture);

    mSeparator = new QFrame(this);
    mSeparator->setObjectName("separator");
    mSeparator->setFrameShape(QFrame::HLine);

    mMessage = new MessageLabel(this);
    connect(mMessage, &QTextEdit::copyAvailable,
            MenuBar::instance(this), &MenuBar::updateCutCopyPaste);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(header);
    layout->addWidget(mSeparator);
    layout->addWidget(mMessage);

    connect(&mMgr, &QNetworkAccessManager::finished,
            this, &CommitDetail::setPicture);

    RepoView *view = RepoView::parentView(this);
    connect(mHash, &QLabel::linkActivated, view, &RepoView::visitLink);
    connect(mParents, &QLabel::linkActivated, view, &RepoView::visitLink);

    connect(&mWatcher, &QFutureWatcher<QString>::finished, this, [this] {
      QString result = mWatcher.result();
      if (result.contains('+'))
        mRefs->appendLabel({result, false, true});
    });

    // Respond to reference changes.
    auto resetRefs = [this] {
      RepoView *view = RepoView::parentView(this);
      setReferences(view->commits());
    };

    git::RepositoryNotifier *notifier = view->repo().notifier();
    connect(notifier, &git::RepositoryNotifier::referenceAdded, resetRefs);
    connect(notifier, &git::RepositoryNotifier::referenceRemoved, resetRefs);
    connect(notifier, &git::RepositoryNotifier::referenceUpdated, resetRefs);
  }

  void setReferences(const QList<git::Commit> &commits)
  {
    QList<Badge::Label> refs;
    foreach (const git::Commit &commit, commits) {
      foreach (const git::Reference &ref, commit.refs())
        refs.append({ref.name(), ref.isHead(), ref.isTag()});
    }

    mRefs->setLabels(refs);

    // Compute description asynchronously.
    if (commits.size() == 1)
      mWatcher.setFuture(
        QtConcurrent::run(commits.first(), &git::Commit::description));
  }

  void setCommits(const QList<git::Commit> &commits)
  {
    // Clear fields.
    mHash->setText(QString());
    mAuthorDate->setDate(QString());
    mAuthorDate->setAuthor(QString());
    mParents->setText(QString());
    mMessage->setPlainText(QString());
    mPicture->setPixmap(QPixmap());

    mParents->setVisible(false);
    mSeparator->setVisible(false);
    mMessage->setVisible(false);

    // Reset references.
    setReferences(commits);

    if (commits.isEmpty())
      return;

    // Show range details.
    if (commits.size() > 1) {
      git::Commit last = commits.last();
      git::Commit first = commits.first();

      // Add names.
      QSet<QString> names;
      foreach (const git::Commit &commit, commits)
        names.insert(kBoldFmt.arg(commit.author().name()));
      QStringList list = names.values();
      if (list.size() > 3)
        list = list.mid(0, 3) << kBoldFmt.arg("...");
      mAuthorDate->setAuthor(list.join(", "));

      // Set date range.
      QDate lastDate = last.committer().date().date();
      QDate firstDate = first.committer().date().date();
      QString lastDateStr = lastDate.toString(Qt::DefaultLocaleShortDate);
      QString firstDateStr = firstDate.toString(Qt::DefaultLocaleShortDate);
      QString dateStr = (lastDate == firstDate) ? lastDateStr :
        kDateRangeFmt.arg(lastDateStr, firstDateStr);
      mAuthorDate->setDate(brightText(dateStr));

      // Set id range.
      QUrl lastUrl;
      lastUrl.setScheme("id");
      lastUrl.setPath(last.id().toString());
      QString lastId = kLinkFmt.arg(lastUrl.toString(), last.shortId());

      QUrl firstUrl;
      firstUrl.setScheme("id");
      firstUrl.setPath(first.id().toString());
      QString firstId = kLinkFmt.arg(firstUrl.toString(), first.shortId());

      QString range = kRangeFmt.arg(lastId, firstId);
      mHash->setText(brightText(tr("Range:")) + " " + range);

      // Remember the range.
      mId = kRangeFmt.arg(last.id().toString(), first.id().toString());

      return;
    }

    // Show commit details.
    mParents->setVisible(true);
    mSeparator->setVisible(true);
    mMessage->setVisible(true);

    // Populate details.
    git::Commit commit = commits.first();
    git::Signature author = commit.author();
    QDateTime date = commit.committer().date();
    mHash->setText(brightText(tr("Id:")) + " " + commit.shortId());
    mAuthorDate->setDate(brightText(date.toString(Qt::DefaultLocaleLongDate)));
    mAuthorDate->setAuthor(kAuthorFmt.arg(author.name(), author.email()));

    QStringList parents;
    foreach (const git::Commit &parent, commit.parents()) {
      QUrl url;
      url.setScheme("id");
      url.setPath(parent.id().toString());
      parents.append(kLinkFmt.arg(url.toString(), parent.shortId()));
    }

    QString initial = kItalicFmt.arg(tr("initial commit"));
    QString text = parents.isEmpty() ? initial : parents.join(", ");
    mParents->setText(brightText("Parents:") + " " + text);

    QString msg = commit.message(git::Commit::SubstituteEmoji).trimmed();
    mMessage->setPlainText(msg);

    int size = kSize * window()->windowHandle()->devicePixelRatio();
    QByteArray email = commit.author().email().trimmed().toLower().toUtf8();
    QByteArray hash = QCryptographicHash::hash(email, QCryptographicHash::Md5);

    // Check the cache first.
    QByteArray key = hash.toHex() + '@' + QByteArray::number(size);
    mPicture->setPixmap(mCache.value(key));

    // Request the image from gravatar.
    if (!mCache.contains(key)) {
      QUrl url(kUrl.arg(QString::fromUtf8(hash.toHex()), QString::number(size)));
      QNetworkReply *reply = mMgr.get(QNetworkRequest(url));
      reply->setProperty(kCacheKey, key);
    }

    // Remember the id.
    mId = commit.id().toString();
  }

  void setPicture(QNetworkReply *reply)
  {
    // Load source.
    QPixmap source;
    source.loadFromData(reply->readAll());

    // Render clipped to circle.
    QPixmap pixmap(source.size());
    pixmap.fill(Qt::transparent);

    // Clip to path. The region overload doesn't antialias.
    QPainterPath path;
    path.addEllipse(pixmap.rect());

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, source);

    // Cache the transformed pixmap.
    pixmap.setDevicePixelRatio(window()->windowHandle()->devicePixelRatio());
    mCache.insert(reply->property(kCacheKey).toByteArray(), pixmap);
    mPicture->setPixmap(pixmap);
    reply->deleteLater();
  }

  void cancelBackgroundTasks()
  {
    // Just wait.
    mWatcher.waitForFinished();
  }

private:
  Badge *mRefs;
  QLabel *mHash;
  QLabel *mParents;
  QLabel *mPicture;
  QFrame *mSeparator;
  QTextEdit *mMessage;
  AuthorDate *mAuthorDate;

  QString mId;
  QNetworkAccessManager mMgr;
  QMap<QByteArray,QPixmap> mCache;
  QFutureWatcher<QString> mWatcher;
};

class CommitEditor : public QFrame
{
  Q_OBJECT

public:
  CommitEditor(const git::Repository &repo, QWidget *parent = nullptr)
    : QFrame(parent)
  {
    git::Config config = repo.appConfig();

    TemplateButton *templateButton = new TemplateButton(config, this);
    templateButton->setText(tr("T"));
    connect(templateButton, &TemplateButton::templateChanged, this, &CommitEditor::applyTemplate);

    QLabel *label = new QLabel(tr("<b>Commit Message:</b>"), this);
    mStatus = new QLabel(QString(), this);

    QHBoxLayout *labelLayout = new QHBoxLayout;
    labelLayout->addWidget(templateButton);
    labelLayout->addWidget(label);
    labelLayout->addStretch();
    labelLayout->addWidget(mStatus);

    mMessage = new QTextEdit(this);
    mMessage->setAcceptRichText(false);
    mMessage->setObjectName("MessageEditor");
    mMessage->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    connect(mMessage, &QTextEdit::textChanged, [this] {
      mPopulate = false;

      bool empty = mMessage->toPlainText().isEmpty();
      if (mEditorEmpty != empty)
        updateButtons();
      mEditorEmpty = empty;

      mPopulate = mMessage->toPlainText().isEmpty();
    });

    // Update menu items.
    MenuBar *menuBar = MenuBar::instance(this);
    connect(mMessage, &QTextEdit::undoAvailable,
            menuBar, &MenuBar::updateUndoRedo);
    connect(mMessage, &QTextEdit::redoAvailable,
            menuBar, &MenuBar::updateUndoRedo);
    connect(mMessage, &QTextEdit::copyAvailable,
            menuBar, &MenuBar::updateCutCopyPaste);

    QVBoxLayout *messageLayout = new QVBoxLayout;
    messageLayout->setContentsMargins(12,8,0,0);
    messageLayout->addLayout(labelLayout);
    messageLayout->addWidget(mMessage);

    mStage = new QPushButton(tr("Stage All"), this);
    mStage->setObjectName("StageAll");
    connect(mStage, &QPushButton::clicked, this, &CommitEditor::stage);

    mUnstage = new QPushButton(tr("Unstage All"), this);
    connect(mUnstage, &QPushButton::clicked, this, &CommitEditor::unstage);

    mCommit = new QPushButton(tr("Commit"), this);
    mCommit->setDefault(true);
    connect(mCommit, &QPushButton::clicked, this, &CommitEditor::commit);

    // Update buttons on index change.
    connect(repo.notifier(), &git::RepositoryNotifier::indexChanged,
    [this](const QStringList &paths, bool yieldFocus) {
      updateButtons(yieldFocus);
    });

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->setContentsMargins(0,8,12,0);
    buttonLayout->addStretch();
    buttonLayout->addWidget(mStage);
    buttonLayout->addWidget(mUnstage);
    buttonLayout->addWidget(mCommit);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,12);
    layout->addLayout(messageLayout);
    layout->addLayout(buttonLayout);
  }

  void commit()
  {
    // Check for a merge head.
    git::AnnotatedCommit upstream;
    RepoView *view = RepoView::parentView(this);
    if (git::Reference mergeHead = view->repo().lookupRef("MERGE_HEAD"))
      upstream = mergeHead.annotatedCommit();

    if (view->commit(mMessage->toPlainText(), upstream))
      mMessage->clear(); // Clear the message field.
  }

  bool isCommitEnabled() const
  {
    return mCommit->isEnabled();
  }

  void stage()
  {
    mDiff.setAllStaged(true);
  }

  bool isStageEnabled() const
  {
    return mStage->isEnabled();
  }

  void unstage()
  {
    mDiff.setAllStaged(false);
  }

  bool isUnstageEnabled() const
  {
    return mUnstage->isEnabled();
  }

  void setMessage(const QString &message)
  {
    mMessage->setPlainText(message);
    mMessage->selectAll();
  }

  void setDiff(const git::Diff &diff)
  {
    mDiff = diff;
    updateButtons(false);

    // Pre-populate commit editor with the merge message.
    QString msg = RepoView::parentView(this)->repo().message();
    if (!msg.isEmpty())
      mMessage->setPlainText(msg);
  }

public slots:
  void applyTemplate(const QString templ)
  {
    mMessage->setText(templ);
  }

private:
  void updateButtons(bool yieldFocus = true)
  {
    if (!mDiff.isValid()) {
      mStage->setEnabled(false);
      mUnstage->setEnabled(false);
      mCommit->setEnabled(false);
      return;
    }

    QStringList list;
    int staged = 0;
    int partial = 0;
    int conflicted = 0;
    int count = mDiff.count();
    git::Index index = mDiff.index();
    for (int i = 0; i < count; ++i) {
      QString name = mDiff.name(i);
      switch (index.isStaged(name)) {
        case git::Index::Disabled:
        case git::Index::Unstaged:
          break;

        case git::Index::PartiallyStaged:
          list.append(QFileInfo(name).fileName());
          ++partial;
          break;

        case git::Index::Staged:
          list.append(QFileInfo(name).fileName());
          ++staged;
          break;

        case git::Index::Conflicted:
          ++conflicted;
          break;
      }
    }

    if (mPopulate) {
      QSignalBlocker blocker(mMessage);
      (void) blocker;

      QString msg;
      switch (list.size()) {
        case 0:
          break;

        case 1:
          msg = tr("Update %1").arg(list.first());
          break;

        case 2:
          msg = tr("Update %1 and %2").arg(list.first(), list.last());
          break;

        case 3:
          msg = tr("Update %1, %2, and %3").arg(
                  list.at(0), list.at(1), list.at(2));
          break;

        default:
          msg = tr("Update %1, %2, and %3 more files...").arg(
                  list.at(0), list.at(1), QString::number(list.size() - 2));
          break;
      }

      setMessage(msg);
      if (yieldFocus && !mMessage->toPlainText().isEmpty())
        mMessage->setFocus();
    }

    int total = staged + partial + conflicted;
    mStage->setEnabled(count > staged);
    mUnstage->setEnabled(total);
    mCommit->setEnabled(total && !mMessage->document()->isEmpty());

    // Set status text.
    QString status = tr("Nothing staged");
    if (staged || partial || conflicted) {
      QString fmt = (staged == 1 && count == 1) ?
        tr("%1 of %2 file staged") : tr("%1 of %2 files staged");
      QStringList fragments(fmt.arg(staged).arg(count));

      if (partial) {
        QString partialFmt = (partial == 1) ?
          tr("%1 file partially staged") : tr("%1 files partially staged");
        fragments.append(partialFmt.arg(partial));
      }

      if (conflicted) {
        QString conflictedFmt = (conflicted == 1) ?
          tr("%1 unresolved conflict") : tr("%1 unresolved conflicts");
        fragments.append(conflictedFmt.arg(conflicted));
      } else if (mDiff.isConflicted()) {
        fragments.append(tr("all conflicts resolved"));
      }

      status = fragments.join(", ");
    }

    mStatus->setText(brightText(status));

    // Change commit button text for committing a merge.
    git::Repository repo = RepoView::parentView(this)->repo();
    bool merging = (repo.state() == GIT_REPOSITORY_STATE_MERGE);
    mCommit->setText(merging ? tr("Commit Merge") : tr("Commit"));

    // Update menu actions.
    MenuBar::instance(this)->updateRepository();
  }

  git::Diff mDiff;

  QLabel *mStatus;
  QTextEdit *mMessage;
  QPushButton *mStage;
  QPushButton *mUnstage;
  QPushButton *mCommit;

  bool mEditorEmpty = true;
  bool mPopulate = true;
};

} // anon. namespace

ContentWidget::ContentWidget(QWidget *parent)
  : QWidget(parent)
{}

ContentWidget::~ContentWidget() {}

DetailView::DetailView(const git::Repository &repo, QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);

  mDetail = new StackedWidget(this);
  mDetail->setVisible(false);
  layout->addWidget(mDetail);

  mDetail->addWidget(new CommitDetail(this));
  mDetail->addWidget(new CommitEditor(repo, this));

  mContent = new QStackedWidget(this);
  layout->addWidget(mContent, 1);

  mContent->addWidget(new DiffWidget(repo, this));
  mContent->addWidget(new TreeWidget(repo, this));
  mContent->addWidget(new DoubleTreeWidget(repo, this));
}

DetailView::~DetailView() {}

void DetailView::commit()
{
  Q_ASSERT(isCommitEnabled());
  static_cast<CommitEditor *>(mDetail->currentWidget())->commit();
}

bool DetailView::isCommitEnabled() const
{
  QWidget *widget = mDetail->currentWidget();
  return (mDetail->currentIndex() == EditorIndex &&
          static_cast<CommitEditor *>(widget)->isCommitEnabled());
}

void DetailView::stage()
{
  Q_ASSERT(isStageEnabled());
  static_cast<CommitEditor *>(mDetail->currentWidget())->stage();
}

bool DetailView::isStageEnabled() const
{
  QWidget *widget = mDetail->currentWidget();
  return (mDetail->currentIndex() == EditorIndex &&
          static_cast<CommitEditor *>(widget)->isStageEnabled());
}

void DetailView::unstage()
{
  Q_ASSERT(isUnstageEnabled());
  static_cast<CommitEditor *>(mDetail->currentWidget())->unstage();
}

bool DetailView::isUnstageEnabled() const
{
  QWidget *widget = mDetail->currentWidget();
  return (mDetail->currentIndex() == EditorIndex &&
          static_cast<CommitEditor *>(widget)->isUnstageEnabled());
}

RepoView::ViewMode DetailView::viewMode() const
{
  return static_cast<RepoView::ViewMode>(mContent->currentIndex());
}

void DetailView::setViewMode(RepoView::ViewMode mode, bool spontaneous)
{
  if (mode == mContent->currentIndex())
    return;

  mContent->setCurrentIndex(mode);

  // Emit own signal so that the view can respond *after* index change.
  emit viewModeChanged(mode, spontaneous);
}

QString DetailView::file() const
{
  return static_cast<ContentWidget *>(mContent->currentWidget())->selectedFile();
}

void DetailView::setCommitMessage(const QString &message)
{
  static_cast<CommitEditor *>(mDetail->widget(EditorIndex))->setMessage(message);
}

void DetailView::setDiff(
  const git::Diff &diff,
  const QString &file,
  const QString &pathspec)
{
  RepoView *view = RepoView::parentView(this);
  QList<git::Commit> commits = view->commits();

  mDetail->setCurrentIndex(commits.isEmpty() ? EditorIndex : CommitIndex);
  mDetail->setVisible(diff.isValid());

  if (commits.isEmpty()) {
    static_cast<CommitEditor *>(mDetail->currentWidget())->setDiff(diff);
  } else {
    static_cast<CommitDetail *>(mDetail->currentWidget())->setCommits(commits);
  }

  ContentWidget *cw = static_cast<ContentWidget *>(mContent->currentWidget());
  cw->setDiff(diff, file, pathspec);

  // Update menu actions.
  MenuBar::instance(this)->updateRepository();
}

void DetailView::cancelBackgroundTasks()
{
  CommitDetail *cd = static_cast<CommitDetail *>(mDetail->widget(CommitIndex));
  cd->cancelBackgroundTasks();

  ContentWidget *cw = static_cast<ContentWidget *>(mContent->currentWidget());
  cw->cancelBackgroundTasks();
}

void DetailView::find()
{
  static_cast<ContentWidget *>(mContent->currentWidget())->find();
}

void DetailView::findNext()
{
  static_cast<ContentWidget *>(mContent->currentWidget())->findNext();
}

void DetailView::findPrevious()
{
  static_cast<ContentWidget *>(mContent->currentWidget())->findPrevious();
}

#include "DetailView.moc"
