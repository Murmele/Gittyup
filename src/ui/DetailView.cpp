//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//          Copyright (c) 2023, Gittyup Contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DetailView.h"
#include "Badge.h"
#include "MenuBar.h"
#include "TreeWidget.h"
#include "DoubleTreeWidget.h"
#include "TreeWidget.h"
#include "CommitEditor.h"
#include "conf/Settings.h"
#include "git/Commit.h"
#include "git/Config.h"
#include "git/Diff.h"
#include "git/Repository.h"
#include "git/Signature.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QActionGroup>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
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

QString brightText(const QString &text) {
  return kAltFmt.arg(QPalette().color(QPalette::BrightText).name(), text);
}

class MessageLabel : public QTextEdit {
public:
  MessageLabel(QWidget *parent = nullptr) : QTextEdit(parent) {
    setObjectName("MessageLabel");
    setFrameShape(QFrame::NoFrame);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setReadOnly(true);
    document()->setDocumentMargin(0);

    // Notify the layout system when size hint changes.
    connect(document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged, this,
            &MessageLabel::updateGeometry);
  }

protected:
  QSize minimumSizeHint() const override {
    QSize size = QTextEdit::minimumSizeHint();
    return QSize(size.width(), fontMetrics().lineSpacing());
  }

  QSize viewportSizeHint() const override {
    // Choose the smaller of the height of the document or five lines.
    QSize size = QTextEdit::viewportSizeHint();
    int height = document()->documentLayout()->documentSize().height();
    return QSize(size.width(), qMin(height, 5 * fontMetrics().lineSpacing()));
  }
};

class StackedWidget : public QStackedWidget {
public:
  StackedWidget(QWidget *parent = nullptr) : QStackedWidget(parent) {}

  QSize sizeHint() const override { return currentWidget()->sizeHint(); }

  QSize minimumSizeHint() const override {
    return currentWidget()->minimumSizeHint();
  }
};

class AuthorCommitterDate : public QWidget {
public:
  AuthorCommitterDate(QWidget *parent = nullptr) : QWidget(parent) {
    mAuthor = new QLabel(this);
    mAuthor->setTextInteractionFlags(kTextFlags);

    mCommitter = new QLabel(this);
    mCommitter->setTextInteractionFlags(kTextFlags);

    mDate = new QLabel(this);
    mDate->setTextInteractionFlags(kTextFlags);

    mHorizontalSpacing =
        style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    mVerticalSpacing = style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
  }

  void moveEvent(QMoveEvent *event) override { updateLayout(); }

  void resizeEvent(QResizeEvent *event) override { updateLayout(); }

  QSize sizeHint() const override {
    QSize date = mDate->sizeHint();
    QSize author = mAuthor->sizeHint();
    QSize committer = mCommitter->sizeHint();
    int width = author.width() + date.width() + mHorizontalSpacing;
    int height;
    if (mSameAuthorCommitter)
      height = qMax(qMax(author.height(), committer.height()), date.height());
    else
      height = qMax(author.height(), date.height()) + committer.height() +
               mVerticalSpacing;
    return QSize(width, height);
  }

  QSize minimumSizeHint() const override {
    QSize date = mDate->minimumSizeHint();
    QSize author = mAuthor->minimumSizeHint();
    QSize committer = mAuthor->minimumSizeHint();
    int width = qMax(qMax(author.width(), committer.width()), date.width());
    int height;
    if (mSameAuthorCommitter)
      height = qMax(qMax(author.height(), committer.height()), date.height());
    else
      height = qMax(author.height(), date.height()) + committer.height() +
               mVerticalSpacing;
    return QSize(width, height);
  }

  bool hasHeightForWidth() const override { return true; }

  int heightForWidth(int width) const override {
    int date = mDate->sizeHint().height();
    int author = mAuthor->sizeHint().height();
    int committer = mCommitter->sizeHint().height();
    bool wrapped = (width < sizeHint().width());
    int unwrappedHeight =
        mSameAuthorCommitter
            ? qMax(committer, qMax(author, date))
            : qMax(author + committer + mVerticalSpacing, date);
    return wrapped ? (author + committer + date + 2 * mVerticalSpacing)
                   : unwrappedHeight;
  }

  void setAuthorCommitter(const QString &author, const QString &committer) {
    mSameAuthorCommitter = author == committer;
    if (mSameAuthorCommitter) {
      mAuthor->setText(tr("Author/Committer: ") + author);
      mAuthor->adjustSize();
      mCommitter->setVisible(false);
    } else {
      mAuthor->setText(tr("Author: ") + author);
      mAuthor->adjustSize();
      mCommitter->setText(tr("Committer: ") + committer);
      mCommitter->adjustSize();
      mCommitter->setVisible(true);
    }
    updateLayout();
  }

  void setDate(const QString &date) {
    mDate->setText(date);
    mDate->adjustSize();
    updateLayout();
  }

private:
  void updateLayout() {
    mAuthor->move(0, 0);
    if (mCommitter->isVisible())
      mCommitter->move(0, mAuthor->height() + mVerticalSpacing);

    bool wrapped = (width() < sizeHint().width());
    int x = wrapped ? 0 : width() - mDate->width();
    int y = wrapped ? mAuthor->height() + mCommitter->height() +
                          2 * mVerticalSpacing
                    : 0;
    mDate->move(x, y);
    updateGeometry();
  }

  QLabel *mAuthor;
  QLabel *mCommitter;
  QLabel *mDate;

  int mHorizontalSpacing;
  int mVerticalSpacing;
  bool mSameAuthorCommitter{false};
};

class CommitDetail : public QFrame {
  Q_OBJECT

public:
  CommitDetail(QWidget *parent = nullptr) : QFrame(parent) {
    mAuthorCommitterDate = new AuthorCommitterDate(this);

    mHash = new QLabel(this);
    mHash->setTextInteractionFlags(kTextFlags);

    QToolButton *copy = new QToolButton(this);
    copy->setText(tr("Copy"));
    connect(copy, &QToolButton::clicked,
            [this] { QApplication::clipboard()->setText(mId); });

    mRefs = new Badge(QList<Badge::Label>(), this);
    mParents = new QLabel(this);
    mParents->setTextInteractionFlags(kTextFlags);

    QHBoxLayout *line3 = new QHBoxLayout;
    line3->addWidget(mHash);
    line3->addWidget(copy);
    line3->addWidget(mParents);
    line3->addStretch();
    line3->addWidget(mRefs);

    QVBoxLayout *details = new QVBoxLayout;
    details->setSpacing(6);
    details->addWidget(mAuthorCommitterDate); // line 1 + 2
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
    connect(mMessage, &QTextEdit::copyAvailable, MenuBar::instance(this),
            &MenuBar::updateCutCopyPaste);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(header);
    layout->addWidget(mSeparator);
    layout->addWidget(mMessage);

    connect(&mMgr, &QNetworkAccessManager::finished, this,
            &CommitDetail::setPicture);

    RepoView *view = RepoView::parentView(this);
    connect(mHash, &QLabel::linkActivated, view, &RepoView::visitLink);
    connect(mParents, &QLabel::linkActivated, view, &RepoView::visitLink);

    connect(&mWatcher, &QFutureWatcher<QString>::finished, this, [this] {
      QString result = mWatcher.result();
      if (result.contains('+'))
        mRefs->appendLabel({Badge::Label::Type::Ref, result, false, true});
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

  void setReferences(const QList<git::Commit> &commits) {
    QList<Badge::Label> refs;
    foreach (const git::Commit &commit, commits) {
      foreach (const git::Reference &ref, commit.refs())
        refs.append(
            {Badge::Label::Type::Ref, ref.name(), ref.isHead(), ref.isTag()});
    }

    mRefs->setLabels(refs);

    // Compute description asynchronously.
    if (commits.size() == 1)
      mWatcher.setFuture(
          QtConcurrent::run(&git::Commit::description, commits.first()));
  }

  void setCommits(const QList<git::Commit> &commits) {
    // Clear fields.
    mHash->setText(QString());
    mAuthorCommitterDate->setDate(QString());
    mAuthorCommitterDate->setAuthorCommitter(QString(), QString());
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
      QSet<QString> authors, committers;
      foreach (const git::Commit &commit, commits) {
        authors.insert(kBoldFmt.arg(commit.author().name()));
        committers.insert(kBoldFmt.arg(commit.committer().name()));
      }
      QStringList author = authors.values();
      if (author.size() > 3)
        author = author.mid(0, 3) << kBoldFmt.arg("...");
      QStringList committer = committers.values();
      if (committer.size() > 3)
        committer = committer.mid(0, 3) << kBoldFmt.arg("...");
      mAuthorCommitterDate->setAuthorCommitter(author.join(", "),
                                               committer.join(", "));

      // Set date range.
      QDate lastDate = last.committer().date().toLocalTime().date();
      QDate firstDate = first.committer().date().toLocalTime().date();
      QString lastDateStr = QLocale().toString(lastDate, QLocale::ShortFormat);
      QString firstDateStr =
          QLocale().toString(firstDate, QLocale::ShortFormat);
      QString dateStr = (lastDate == firstDate)
                            ? lastDateStr
                            : kDateRangeFmt.arg(lastDateStr, firstDateStr);
      mAuthorCommitterDate->setDate(brightText(dateStr));

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
    git::Signature committer = commit.committer();
    QDateTime date = commit.committer().date().toLocalTime();
    mHash->setText(brightText(tr("Id:")) + " " + commit.shortId());
    mAuthorCommitterDate->setDate(
        brightText(QLocale().toString(date, QLocale::LongFormat)));
    mAuthorCommitterDate->setAuthorCommitter(
        kAuthorFmt.arg(author.name(), author.email()),
        kAuthorFmt.arg(committer.name(), committer.email()));

    QStringList parents;
    foreach (const git::Commit &parent, commit.parents()) {
      QUrl url;
      url.setScheme("id");
      url.setPath(parent.id().toString());
      parents.append(kLinkFmt.arg(url.toString(), parent.shortId()));
    }

    QString initial = kItalicFmt.arg(tr("initial commit"));
    QString text = parents.isEmpty() ? initial : parents.join(", ");
    mParents->setText(brightText(tr("Parents:")) + " " + text);

    QString msg = commit.message(git::Commit::SubstituteEmoji).trimmed();
    mMessage->setPlainText(msg);

    const bool showAvatars =
        Settings::instance()->value(Setting::Id::ShowAvatars).toBool();
    if (showAvatars) {
      auto w = window();
      auto w_handler = w->windowHandle();

      int size = kSize * w_handler->devicePixelRatio();
      QByteArray email = commit.author().email().trimmed().toLower().toUtf8();
      QByteArray hash =
          QCryptographicHash::hash(email, QCryptographicHash::Md5);

      // Check the cache first.
      QByteArray key = hash.toHex() + '@' + QByteArray::number(size);
      mPicture->setPixmap(mCache.value(key));

      // Request the image from gravatar.
      if (!mCache.contains(key)) {
        QUrl url(
            kUrl.arg(QString::fromUtf8(hash.toHex()), QString::number(size)));
        QNetworkReply *reply = mMgr.get(QNetworkRequest(url));
        reply->setProperty(kCacheKey, key);
      }
    }

    // Remember the id.
    mId = commit.id().toString();
  }

  void setPicture(QNetworkReply *reply) {
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

  void cancelBackgroundTasks() {
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
  AuthorCommitterDate *mAuthorCommitterDate;

  QString mId;
  QNetworkAccessManager mMgr;
  QMap<QByteArray, QPixmap> mCache;
  QFutureWatcher<QString> mWatcher;
};

} // namespace

ContentWidget::ContentWidget(QWidget *parent) : QWidget(parent) {}

ContentWidget::~ContentWidget() {}

DetailView::DetailView(const git::Repository &repo, QWidget *parent)
    : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  mDetail = new StackedWidget(this);
  mDetail->setVisible(false);
  layout->addWidget(mDetail);

  mDetail->addWidget(new CommitDetail(this));

  mAuthorLabel = new QLabel(this);
  mAuthorLabel->setTextFormat(Qt::TextFormat::RichText);
  connect(mAuthorLabel, &QLabel::linkActivated, this,
          &DetailView::authorLinkActivated);
  updateAuthor();

  mCommitEditor = new CommitEditor(repo, this);

  auto editorFrame = new QWidget(this);
  auto editor = new QVBoxLayout(editorFrame);
  editor->addWidget(mAuthorLabel);
  editor->addWidget(mCommitEditor);

  mDetail->addWidget(editorFrame);

  mContent = new QStackedWidget(this);
  layout->addWidget(mContent, 1);

  mContent->addWidget(new DoubleTreeWidget(repo, this));
  mContent->addWidget(new TreeWidget(repo, this));
}

DetailView::~DetailView() {}

void DetailView::commit(bool force) {
  Q_ASSERT(isCommitEnabled());
  mCommitEditor->commit(force);
}

bool DetailView::isCommitEnabled() const {
  return (mDetail->currentIndex() == EditorIndex &&
          mCommitEditor->isCommitEnabled());
}

bool DetailView::isRebaseContinueVisible() const {
  return (mDetail->currentIndex() == EditorIndex &&
          mCommitEditor->isRebaseContinueVisible());
}

bool DetailView::isRebaseAbortVisible() const {
  return (mDetail->currentIndex() == EditorIndex &&
          mCommitEditor->isRebaseAbortVisible());
}

void DetailView::stage() {
  Q_ASSERT(isStageEnabled());
  mCommitEditor->stage();
}

bool DetailView::isStageEnabled() const {
  return (mDetail->currentIndex() == EditorIndex &&
          mCommitEditor->isStageEnabled());
}

void DetailView::unstage() {
  Q_ASSERT(isUnstageEnabled());
  mCommitEditor->unstage();
}

bool DetailView::isUnstageEnabled() const {
  return (mDetail->currentIndex() == EditorIndex &&
          mCommitEditor->isUnstageEnabled());
}

RepoView::ViewMode DetailView::viewMode() const {
  return static_cast<RepoView::ViewMode>(mContent->currentIndex());
}

void DetailView::setViewMode(RepoView::ViewMode mode, bool spontaneous) {
  if (mode == mContent->currentIndex())
    return;

  mContent->setCurrentIndex(mode);

  // Emit own signal so that the view can respond *after* index change.
  emit viewModeChanged(mode, spontaneous);
}

QString DetailView::file() const {
  return static_cast<ContentWidget *>(mContent->currentWidget())
      ->selectedFile();
}

void DetailView::setCommitMessage(const QString &message) {
  mCommitEditor->setMessage(message);
}

QString DetailView::commitMessage() const { return mCommitEditor->message(); }

void DetailView::setDiff(const git::Diff &diff, const QString &file,
                         const QString &pathspec) {
  RepoView *view = RepoView::parentView(this);
  QList<git::Commit> commits = view->commits();

  mDetail->setCurrentIndex(commits.isEmpty() ? EditorIndex : CommitIndex);
  mDetail->setVisible(diff.isValid());

  if (commits.isEmpty()) {
    mCommitEditor->setDiff(diff);
  } else {
    static_cast<CommitDetail *>(mDetail->currentWidget())->setCommits(commits);
  }

  ContentWidget *cw = static_cast<ContentWidget *>(mContent->currentWidget());
  cw->setDiff(diff, file, pathspec);

  // Update menu actions.
  MenuBar::instance(this)->updateRepository();
}

void DetailView::cancelBackgroundTasks() {
  CommitDetail *cd = static_cast<CommitDetail *>(mDetail->widget(CommitIndex));
  cd->cancelBackgroundTasks();

  ContentWidget *cw = static_cast<ContentWidget *>(mContent->currentWidget());
  cw->cancelBackgroundTasks();
}

void DetailView::find() {
  static_cast<ContentWidget *>(mContent->currentWidget())->find();
}

void DetailView::findNext() {
  static_cast<ContentWidget *>(mContent->currentWidget())->findNext();
}

void DetailView::findPrevious() {
  static_cast<ContentWidget *>(mContent->currentWidget())->findPrevious();
}

QString DetailView::overrideUser() const { return mOverrideUser; }

QString DetailView::overrideEmail() const { return mOverrideEmail; }

void DetailView::updateAuthor() {
  git::Config config = RepoView::parentView(this)->repo().gitConfig();

  QString text = "<a href=\"changeAuthor\"><b>" + tr("Author:") + "</b></a> ";

  if (mOverrideUser.isEmpty())
    text += config.value<QString>("user.name").toHtmlEscaped();
  else
    text += mOverrideUser.toHtmlEscaped();

  if (mOverrideEmail.isEmpty())
    text +=
        " &lt;" + config.value<QString>("user.email").toHtmlEscaped() + "&gt;";
  else
    text += " &lt;" + mOverrideEmail.toHtmlEscaped() + "&gt;";

  if (!mOverrideUser.isEmpty() || !mOverrideEmail.isEmpty())
    text += " (<a href=\"reset\">" + tr("reset") + "</a>)";

  mAuthorLabel->setText(text);
}

void DetailView::authorLinkActivated(const QString &href) {
  if (href == "changeAuthor") {
    QDialog *dialog = new QDialog(this);
    QFormLayout *layout = new QFormLayout(dialog);

    layout->addRow(
        new QLabel(tr("Here you can set the author used for committing\n"
                      "These settings will not be saved permanently")));

    QLineEdit *userEdit = new QLineEdit(mOverrideUser, dialog);
    layout->addRow(tr("Author:"), userEdit);

    QLineEdit *emailEdit = new QLineEdit(mOverrideEmail, dialog);
    layout->addRow(tr("Email:"), emailEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(dialog);
    buttons->addButton(QDialogButtonBox::Ok);
    buttons->addButton(QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addRow(buttons);

    connect(dialog, &QDialog::accepted, [this, userEdit, emailEdit]() {
      mOverrideUser = userEdit->text();
      mOverrideEmail = emailEdit->text();
      updateAuthor();
    });

    dialog->setModal(true);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

  } else if (href == "reset") {
    mOverrideUser = "";
    mOverrideEmail = "";
    updateAuthor();
  }
}

#include "DetailView.moc"
