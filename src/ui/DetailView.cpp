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
#include "ContextMenuButton.h"
#include "MenuBar.h"
#include "ContextMenuButton.h"
#include "TreeWidget.h"
#include "TemplateButton.h"
#include "DoubleTreeWidget.h"
#include "SpellChecker.h"
#include "TreeWidget.h"
#include "TemplateButton.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include "git/Config.h"
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

const QString kDictKey = "commit.spellcheck.dict";

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
    mParents->setText(brightText(tr("Parents:")) + " " + text);

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

class TextEdit : public QTextEdit
{
  Q_OBJECT

public:
  explicit TextEdit(QWidget *parent = nullptr)
    : QTextEdit(parent)
  {
    // Spell check with delay timeout.
    connect(&mTimer, &QTimer::timeout, [this] {
      mTimer.stop();
      if (mSpellChecker)
        checkSpelling();
    });

    // Spell check on textchange.
    connect(this, &QTextEdit::textChanged, [this] {
      mTimer.start(500);
    });
  }

  bool setupSpellCheck(
    const QString &dictPath,
    const QString &userDict,
    const QTextCharFormat &spellFormat,
    const QTextCharFormat &ignoredFormat)
  {
    mSpellChecker = new SpellChecker(dictPath, userDict);
    if (!mSpellChecker->isValid()) {
      delete mSpellChecker;
      mSpellChecker = nullptr;
      mSpellList.clear();
      setSelections();
      return false;
    }

    mSpellFormat = spellFormat;
    mIgnoredFormat = ignoredFormat;
    checkSpelling();
    return true;
  }

private:
  void contextMenuEvent(QContextMenuEvent *event) override
  {
    // Check for spell checking enabled and a word under the cursor.
    QTextCursor cursor = cursorForPosition(event->pos());
    cursor.select(QTextCursor::WordUnderCursor);
    QString word = cursor.selectedText();
    if (!mSpellChecker || word.isEmpty()) {
      QTextEdit::contextMenuEvent(event);
      return;
    }

    QMenu *menu = createStandardContextMenu();
    foreach (const QTextEdit::ExtraSelection &es, mSpellList) {
      if (es.cursor == cursor && es.format == mSpellFormat) {

        // Replace standard context menu.
        menu->clear();

        QStringList suggestions = mSpellChecker->suggest(word);
        if (!suggestions.isEmpty()) {
          QMenu *spellReplace = menu->addMenu(tr("Replace..."));
          QMenu *spellReplaceAll = menu->addMenu(tr("Replace All..."));
          foreach (const QString &str, suggestions) {
            QAction *replace = spellReplace->addAction(str);
            connect(replace, &QAction::triggered, [this, event, str] {
              QTextCursor cursor = cursorForPosition(event->pos());
              cursor.select(QTextCursor::WordUnderCursor);
              cursor.insertText(str);
              checkSpelling();
            });

            QAction *replaceAll = spellReplaceAll->addAction(str);
              connect(replaceAll, &QAction::triggered, [this, word, str] {
              QTextCursor cursor(document());
              while (!cursor.atEnd()) {
                cursor.movePosition(QTextCursor::EndOfWord,
                                    QTextCursor::KeepAnchor, 1);
                QString search = wordAt(cursor);
                if (!search.isEmpty() && (search == word) &&
                    !ignoredAt(cursor))
                  cursor.insertText(str);

                cursor.movePosition(QTextCursor::NextWord,
                                    QTextCursor::MoveAnchor, 1);
              }
              checkSpelling();
            });
          }
          menu->addSeparator();
        }

        QAction *spellIgnore = menu->addAction(tr("Ignore"));
        connect(spellIgnore, &QAction::triggered, [this, event] {
          QTextCursor cursor = cursorForPosition(event->pos());
          cursor.select(QTextCursor::WordUnderCursor);

          for (int i = 0; i < mSpellList.count(); i++) {
            QTextEdit::ExtraSelection es = mSpellList.at(i);
            if (es.cursor == cursor) {
              mSpellList.removeAt(i);
              es.format = mIgnoredFormat;
              mSpellList << es;

              setSelections();
              break;
            }
          }
          checkSpelling();
        });

        QAction *spellIgnoreAll = menu->addAction(tr("Ignore All"));
        connect(spellIgnoreAll, &QAction::triggered, [this, word] {
          mSpellChecker->ignoreWord(word);
          checkSpelling();
        });

        QAction *spellAdd = menu->addAction(tr("Add to User Dictionary"));
        connect(spellAdd, &QAction::triggered, [this, word] {
          mSpellChecker->addToUserDict(word);
          checkSpelling();
        });
        break;
      }

      // Ignored words.
      if (es.cursor == cursor && es.format == mIgnoredFormat) {

        // Replace standard context menu.
        menu->clear();

        QAction *spellIgnore = menu->addAction(tr("Do not Ignore"));
        connect(spellIgnore, &QAction::triggered, [this, event] {
          QTextCursor cursor = cursorForPosition(event->pos());
          cursor.select(QTextCursor::WordUnderCursor);

          for (int i = 0; i < mSpellList.count(); i++) {
            QTextEdit::ExtraSelection es = mSpellList.at(i);
            if (es.cursor == cursor) {
              mSpellList.removeAt(i);

              setSelections();
              break;
            }
          }
          checkSpelling();
        });
        break;
      }
    }

    menu->exec(event->globalPos());
    delete menu;
  }

  void keyPressEvent(QKeyEvent *event) override
  {
    QTextEdit::keyPressEvent(event);

    QString text = event->text();
    if (!text.isEmpty()) {
      QChar chr = text.at(0);

      // Spell check:
      //   delayed check while writing
      //   immediate check if space, comma, ... is pressed
      if (chr.isLetter() || chr.isNumber()) {
        mTimer.start(500);
      } else if (mSpellChecker && !event->isAutoRepeat()) {
        checkSpelling();
      }
    }
  }

  void checkSpelling()
  {
    QTextCursor cursor(document());
    mSpellList.clear();

    while (!cursor.atEnd()) {
      cursor.movePosition(QTextCursor::EndOfWord,
                          QTextCursor::KeepAnchor, 1);
      QString word = wordAt(cursor);
      if (!word.isEmpty() && !mSpellChecker->spell(word)) {
        // Highlight the unknown or ignored word.
        QTextEdit::ExtraSelection es;
        es.cursor = cursor;
        es.format = ignoredAt(cursor) ? mIgnoredFormat : mSpellFormat;

        mSpellList << es;
      }
      cursor.movePosition(QTextCursor::NextWord,
                          QTextCursor::MoveAnchor, 1);
    }
    setSelections();
  }

  bool ignoredAt(const QTextCursor &cursor)
  {
    foreach (const QTextEdit::ExtraSelection &es, extraSelections()) {
      if (es.cursor == cursor && es.format == mIgnoredFormat)
        return true;
    }

    return false;
  }

  const QString wordAt(QTextCursor &cursor)
  {
    QString word = cursor.selectedText();

    // For a better recognition of words
    // punctuation etc. does not belong to words.
    while (!word.isEmpty() && !word.at(0).isLetter() &&
           (cursor.anchor() < cursor.position())) {
      int cursorPos = cursor.position();
      cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
      cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
      word = cursor.selectedText();
    }
    return word;
  }

  void setSelections(void)
  {
    QList<QTextEdit::ExtraSelection> esList;
    esList.append(mSpellList);
    setExtraSelections(esList);
  }

  QTimer mTimer;

  SpellChecker *mSpellChecker = nullptr;
  QTextCharFormat mSpellFormat;
  QTextCharFormat mIgnoredFormat;
  QList<QTextEdit::ExtraSelection> mSpellList;
};


/*!
 * \brief The CommitEditor class
 * This widget contains the textedit element for entering the commit message, the buttons
 * for commiting, staging all and unstage all
 */
class CommitEditor : public QFrame
{
  Q_OBJECT

public:
  CommitEditor(const git::Repository &repo, QWidget *parent = nullptr)
    : QFrame(parent), mRepo(repo)
  {
    git::Config config = repo.appConfig();

    TemplateButton *templateButton = new TemplateButton(config, this);
    templateButton->setText(tr("T"));
    connect(templateButton, &TemplateButton::templateChanged, this, &CommitEditor::applyTemplate);

    QLabel *label = new QLabel(tr("<b>Commit Message:</b>"), this);

    // Style and color setup for checks.
    mSpellError.setUnderlineColor(Application::theme()->commitEditor(
                                    Theme::CommitEditor::SpellError));
    mSpellError.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    mSpellIgnore.setUnderlineColor(Application::theme()->commitEditor(
                                     Theme::CommitEditor::SpellIgnore));
    mSpellIgnore.setUnderlineStyle(QTextCharFormat::WaveUnderline);

    // Spell check configuration
    mDictName = repo.appConfig().value<QString>(kDictKey, "system");
    mDictPath = Settings::dictionariesDir().path();
    mUserDict = Settings::userDir().path() + "/user.dic";
    QFile userDict(mUserDict);
    if (!userDict.exists()) {
      userDict.open(QIODevice::WriteOnly);
      userDict.close();
    }

    // Find installed Dictionaries.
    QDir dictDir = Settings::dictionariesDir();
    QStringList dictNameList = dictDir.entryList({"*.dic"},
                                                 QDir::Files,
                                                 QDir::Name);
    dictNameList.replaceInStrings(".dic", "");

    // Spell check language menu actions.
    bool selected = false;
    QList<QAction*> actionList;
    foreach (const QString &dict, dictNameList) {
      QLocale locale(dict);

      // Convert language_COUNTRY format from dictionary filename to string
      QString language = QLocale::languageToString(locale.language());
      QString country = QLocale::countryToString(locale.country());
      QString text;

      if (language != "C") {
        text = language;
        if (country != "Default")
          text.append(QString(" (%1)").arg(country));
      } else {
        text = dict;
        while (text.count("_") > 1)
          text = text.left(text.lastIndexOf("_"));
      }

      QAction *action = new QAction(text);
      action->setData(dict);
      action->setCheckable(true);
      actionList.append(action);

      if (dict.startsWith(mDictName)) {
        action->setChecked(true);
        mDictName = dict;
        selected = true;
      }
    }

    // Sort menu entries alphabetical.
    std::sort(actionList.begin(), actionList.end(),
    [actionList](QAction *la, QAction *ra) {
      return la->text() < ra->text();
    });

    QActionGroup *dictActionGroup = new QActionGroup(this);
    dictActionGroup->setExclusive(true);
    foreach (QAction *action, actionList)
      dictActionGroup->addAction(action);

    // No dictionary set: select dictionary for system language and country
    if ((!selected) && (mDictName != "none")) {
      QString name = QLocale::system().name();
      foreach (QAction *action, dictActionGroup->actions()) {
        if (action->data().toString().startsWith(name)) {
          action->setChecked(true);
          mDictName = action->data().toString();
          selected = true;
          break;
        }
      }

      // Fallback: ignore country (e.g.: use de_DE instead of de_AT)
      if (!selected) {
        foreach (QAction *action, dictActionGroup->actions()) {
          if (action->data().toString().startsWith(name.left(2))) {
            action->setChecked(true);
            mDictName = action->data().toString();
            selected = true;
            break;
          }
        }
      }
    }

    connect(dictActionGroup, &QActionGroup::triggered,
    [this](QAction *action) {
      QString dict = action->data().toString();
      if (mDictName == dict) {
        action->setChecked(false);

        // Disable spell checking.
        mDictName = "none";
      } else {
        mDictName = dict;
      }

      // Apply changes, disable invalid dictionary.
      QString path = mDictPath + "/" + mDictName;
      if (!mMessage->setupSpellCheck(
            path, mUserDict, mSpellError, mSpellIgnore) &&
          mDictName != "none") {
        QMessageBox mb(QMessageBox::Critical,
                       tr("Spell Check Language"),
                       tr("The dictionary '%1' is invalid").arg(action->text()));
        mb.setInformativeText(tr("Spell checking is disabled."));
        mb.setDetailedText(tr("The choosen dictionary '%1.dic' is not a "
                              "valid hunspell dictionary.").arg(mDictName));
        mb.exec();

        action->setChecked(false);
        action->setEnabled(false);
        action->setToolTip(tr("Invalid dictionary '%1.dic'").arg(mDictName));
        mDictName = "none";
      }

      // Save settings.
      mRepo.appConfig().setValue(kDictKey, mDictName);
    });

    mStatus = new QLabel(QString(), this);

    // Context button.
    ContextMenuButton *button = new ContextMenuButton(this);
    QMenu *menu = new QMenu(this);
    button->setMenu(menu);

    // Spell check language menu.
    QMenu *spellCheckLanguage = menu->addMenu(tr("Spell Check Language"));
    spellCheckLanguage->setEnabled(!dictNameList.isEmpty());
    spellCheckLanguage->setToolTipsVisible(true);
    spellCheckLanguage->addActions(dictActionGroup->actions());

    // User dictionary.
    menu->addAction(tr("Edit User Dictionary"), [this] {
      RepoView *view = RepoView::parentView(this);
      view->openEditor(mUserDict);
    });

    QHBoxLayout *labelLayout = new QHBoxLayout;
    labelLayout->addWidget(templateButton);
    labelLayout->addWidget(label);
    labelLayout->addStretch();
    labelLayout->addWidget(mStatus);
    labelLayout->addWidget(button);

    mMessage = new TextEdit(this);
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

    // Setup spell check.
    if (mDictName != "none") {
      QString path = mDictPath + "/" + mDictName;
      if (!mMessage->setupSpellCheck(
            path, mUserDict, mSpellError, mSpellIgnore)) {
        foreach (QAction *action, dictActionGroup->actions()) {
          action->setChecked(false);
          if (mDictName == action->data().toString()) {
            action->setEnabled(false);
            action->setToolTip(tr("Invalid dictionary '%1.dic'")
                                 .arg(mDictName));
          }
        }
        mDictName = "none";
      }
    }

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
  void applyTemplate(QString& templ)
  {
	auto index = templ.indexOf(TemplateButton::cursorPositionString);
	if (index < 0)
		index = templ.length();

	templ.replace("%|", "");
    mMessage->setText(templ);
	auto cursor = mMessage->textCursor();
	cursor.setPosition(index);
	mMessage->setTextCursor(cursor);
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

  git::Repository mRepo;
  git::Diff mDiff;

  QLabel *mStatus;
  TextEdit *mMessage;
  QPushButton *mStage;
  QPushButton *mUnstage;
  QPushButton *mCommit;

  bool mEditorEmpty = true;
  bool mPopulate = true;

  QString mDictName;
  QString mDictPath;
  QString mUserDict;

  QTextCharFormat mSpellError;
  QTextCharFormat mSpellIgnore;
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

  mAuthorLabel = new QLabel(this);
  mAuthorLabel->setTextFormat(Qt::TextFormat::RichText);
  connect(mAuthorLabel, &QLabel::linkActivated, this, &DetailView::authorLinkActivated);
  updateAuthor();
  layout->addWidget(mAuthorLabel);

  mDetail = new StackedWidget(this);
  mDetail->setVisible(false);
  layout->addWidget(mDetail);

  mDetail->addWidget(new CommitDetail(this));
  mDetail->addWidget(new CommitEditor(repo, this));

  mContent = new QStackedWidget(this);
  layout->addWidget(mContent, 1);

  mContent->addWidget(new DoubleTreeWidget(repo, this));
  mContent->addWidget(new TreeWidget(repo, this));
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

QString DetailView::overrideUser() const
{
  return mOverrideUser;
}

QString DetailView::overrideEmail() const
{
  return mOverrideEmail;
}

void DetailView::updateAuthor()
{
  git::Config config = RepoView::parentView(this)->repo().config();

  QString text = "<a href=\"changeAuthor\"><b>" + tr("Author:") + "</b></a> ";

  if (mOverrideUser.isEmpty())
    text += config.value<QString>("user.name").toHtmlEscaped();
  else
    text += mOverrideUser.toHtmlEscaped();

  if (mOverrideEmail.isEmpty())
    text += " &lt;" + config.value<QString>("user.email").toHtmlEscaped() + "&gt;";
  else
    text += " &lt;" + mOverrideEmail.toHtmlEscaped() + "&gt;";

  if (!mOverrideUser.isEmpty() || !mOverrideEmail.isEmpty())
    text += " (<a href=\"reset\">" + tr("reset") + "</a>)";

  mAuthorLabel->setText(text);
}

void DetailView::authorLinkActivated(const QString &href)
{
  if (href == "changeAuthor") {
    QDialog *dialog = new QDialog(this);
    QFormLayout *layout = new QFormLayout(dialog);

    layout->addRow(new QLabel(tr(
      "Here you can set the author used for committing\n"
      "These settings will not be saved permanently"
    )));

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
