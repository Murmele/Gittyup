#include "CommitEditor.h"
#include "TemplateButton.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "SpellChecker.h"
#include "ContextMenuButton.h"
#include "MenuBar.h"
#include "RepoView.h"

#include <QLabel>
#include <QPushButton>
#include <QLocale>
#include <QAction>
#include <QMessageBox>
#include <QTextEdit>
#include <QTimer>
#include <QMenu>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace {
const QString kDictKey = "commit.spellcheck.dict";
const QString kAltFmt = "<span style='color: %1'>%2</span>";

QString brightText(const QString &text) {
  return kAltFmt.arg(QPalette().color(QPalette::BrightText).name(), text);
}
} // namespace

class TextEdit : public QTextEdit {
  Q_OBJECT

public:
  explicit TextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {
    // Spell check with delay timeout.
    connect(&mTimer, &QTimer::timeout, [this] {
      mTimer.stop();
      if (mSpellChecker)
        checkSpelling();
    });

    // Spell check on textchange.
    connect(this, &QTextEdit::textChanged, [this] { mTimer.start(500); });
  }

  bool setupSpellCheck(const QString &dictPath, const QString &userDict,
                       const QTextCharFormat &spellFormat,
                       const QTextCharFormat &ignoredFormat) {
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
  void contextMenuEvent(QContextMenuEvent *event) override {
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
                if (!search.isEmpty() && (search == word) && !ignoredAt(cursor))
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

  void keyPressEvent(QKeyEvent *event) override {
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

  void checkSpelling() {
    QTextCursor cursor(document());
    mSpellList.clear();

    while (!cursor.atEnd()) {
      cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
      QString word = wordAt(cursor);
      if (!word.isEmpty() && !mSpellChecker->spell(word)) {
        // Highlight the unknown or ignored word.
        QTextEdit::ExtraSelection es;
        es.cursor = cursor;
        es.format = ignoredAt(cursor) ? mIgnoredFormat : mSpellFormat;

        mSpellList << es;
      }
      cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }
    setSelections();
  }

  bool ignoredAt(const QTextCursor &cursor) {
    foreach (const QTextEdit::ExtraSelection &es, extraSelections()) {
      if (es.cursor == cursor && es.format == mIgnoredFormat)
        return true;
    }

    return false;
  }

  const QString wordAt(QTextCursor &cursor) {
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

  void setSelections(void) {
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

CommitEditor::CommitEditor(const git::Repository &repo, QWidget *parent)
    : QFrame(parent), mRepo(repo) {
  mTemplate = new TemplateButton(this);
  mTemplate->setText(tr("T"));
  connect(mTemplate, &TemplateButton::templateChanged, this,
          [this](const QString &t) {
            QStringList files;
            if (mDiff.isValid()) {
              int count = mDiff.count();
              git::Index index = mDiff.index();
              for (int i = 0; i < count; ++i) {
                QString name = mDiff.name(i);
                switch (index.isStaged(name)) {
                  case git::Index::PartiallyStaged:
                  case git::Index::Staged:
                    files.append(QFileInfo(name).fileName());
                    break;
                  case git::Index::Conflicted:
                  case git::Index::Disabled:
                  case git::Index::Unstaged:
                    break;
                }
              }
            }
            applyTemplate(t, files);
          });

  QLabel *label = new QLabel(tr("<b>Commit Message:</b>"), this);

  // Style and color setup for checks.
  mSpellError.setUnderlineColor(
      Application::theme()->commitEditor(Theme::CommitEditor::SpellError));
  mSpellError.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
  mSpellIgnore.setUnderlineColor(
      Application::theme()->commitEditor(Theme::CommitEditor::SpellIgnore));
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
  QStringList dictNameList =
      dictDir.entryList({"*.dic"}, QDir::Files, QDir::Name);
  dictNameList.replaceInStrings(".dic", "");

  // Spell check language menu actions.
  bool selected = false;
  QList<QAction *> actionList;
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

  connect(dictActionGroup, &QActionGroup::triggered, [this](QAction *action) {
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
    if (!mMessage->setupSpellCheck(path, mUserDict, mSpellError,
                                   mSpellIgnore) &&
        mDictName != "none") {
      QMessageBox mb(QMessageBox::Critical, tr("Spell Check Language"),
                     tr("The dictionary '%1' is invalid").arg(action->text()));
      mb.setInformativeText(tr("Spell checking is disabled."));
      mb.setDetailedText(tr("The choosen dictionary '%1.dic' is not a "
                            "valid hunspell dictionary.")
                             .arg(mDictName));
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
  labelLayout->addWidget(mTemplate);
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
    if (!mMessage->setupSpellCheck(path, mUserDict, mSpellError,
                                   mSpellIgnore)) {
      foreach (QAction *action, dictActionGroup->actions()) {
        action->setChecked(false);
        if (mDictName == action->data().toString()) {
          action->setEnabled(false);
          action->setToolTip(tr("Invalid dictionary '%1.dic'").arg(mDictName));
        }
      }
      mDictName = "none";
    }
  }

  // Update menu items.
  MenuBar *menuBar = MenuBar::instance(this);
  connect(mMessage, &QTextEdit::undoAvailable, menuBar,
          &MenuBar::updateUndoRedo);
  connect(mMessage, &QTextEdit::redoAvailable, menuBar,
          &MenuBar::updateUndoRedo);
  connect(mMessage, &QTextEdit::copyAvailable, menuBar,
          &MenuBar::updateCutCopyPaste);

  QVBoxLayout *messageLayout = new QVBoxLayout;
  messageLayout->setContentsMargins(12, 8, 0, 0);
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

  mRebaseAbort = new QPushButton(tr("Abort rebasing"), this);
  mRebaseAbort->setObjectName("AbortRebase");
  connect(mRebaseAbort, &QPushButton::clicked, this,
          &CommitEditor::abortRebase);

  mRebaseContinue = new QPushButton(tr("Continue rebasing"), this);
  mRebaseContinue->setObjectName("ContinueRebase");
  connect(mRebaseContinue, &QPushButton::clicked, this,
          &CommitEditor::continueRebase);

  mMergeAbort = new QPushButton(tr("Abort Merge"), this);
  connect(mMergeAbort, &QPushButton::clicked, [this] {
    RepoView *view = RepoView::parentView(this);
    view->mergeAbort();
  });

  // Update buttons on index change.
  connect(repo.notifier(), &git::RepositoryNotifier::indexChanged,
          [this](const QStringList &paths, bool yieldFocus) {
            updateButtons(yieldFocus);
          });

  QVBoxLayout *buttonLayout = new QVBoxLayout;
  buttonLayout->setContentsMargins(0, 8, 12, 0);
  buttonLayout->addStretch();
  buttonLayout->addWidget(mStage);
  buttonLayout->addWidget(mUnstage);
  buttonLayout->addWidget(mCommit);
  buttonLayout->addWidget(mRebaseContinue);
  buttonLayout->addWidget(mRebaseAbort);
  buttonLayout->addWidget(mMergeAbort);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 12);
  layout->addLayout(messageLayout);
  layout->addLayout(buttonLayout);
}

void CommitEditor::commit(bool force) {
  // Check for a merge head.
  git::AnnotatedCommit upstream;
  RepoView *view = RepoView::parentView(this);
  if (git::Reference mergeHead = view->repo().lookupRef(
          "MERGE_HEAD")) // TODO: is it possible to use instead of the string
                         // GIT_MERGE_HEAD_FILE?
    upstream = mergeHead.annotatedCommit();

  if (view->commit(mMessage->toPlainText(), upstream, nullptr, force))
    mMessage->clear(); // Clear the message field.
}

void CommitEditor::abortRebase() {
  RepoView *view = RepoView::parentView(this);
  view->abortRebase();
}

void CommitEditor::continueRebase() {
  RepoView *view = RepoView::parentView(this);
  view->continueRebase();
}

bool CommitEditor::isRebaseAbortVisible() const {
  return mRebaseAbort->isVisible();
}

bool CommitEditor::isRebaseContinueVisible() const {
  return mRebaseContinue->isVisible();
}

bool CommitEditor::isCommitEnabled() const { return mCommit->isEnabled(); }

void CommitEditor::stage() { mDiff.setAllStaged(true); }

bool CommitEditor::isStageEnabled() const { return mStage->isEnabled(); }

void CommitEditor::unstage() { mDiff.setAllStaged(false); }

bool CommitEditor::isUnstageEnabled() const { return mUnstage->isEnabled(); }

QString CommitEditor::createFileList(const QStringList &list, int maxFiles) {
  QString msg;

  const int numberFiles = list.size();

  if (numberFiles == 0 || maxFiles == 0)
    return QStringLiteral("");

  if (numberFiles == 1) {
    return tr("%1").arg(list.first());
  } else if (numberFiles == 2 && maxFiles >= 2) {
    return tr("%1 and %2").arg(list.first(), list.last());
  } else if (numberFiles == 3 && maxFiles >= 3) {
    return tr("%1, %2, and %3").arg(list.at(0), list.at(1), list.at(2));
  }

  // numberFiles > 3 || maxFiles < numberFiles
  const int s = qMin(numberFiles, maxFiles) - 1;

  for (int i = 0; i < s; i++) {
    msg += list.at(i) + QStringLiteral(", ");
  }

  if (numberFiles > s + 1) {
    msg += list.at(s) + QStringLiteral(", ");
    const int remainingFiles = numberFiles - s - 1;
    msg += QStringLiteral("and %1 more file").arg(remainingFiles);
    if (remainingFiles > 1)
      msg += QStringLiteral("s");
  } else {
    msg += QStringLiteral("and %1").arg(list.at(s));
  }
  return msg;
}

void CommitEditor::setMessage(const QStringList &files) {
  if (mTemplate->templates().count() > 0) {
    applyTemplate(mTemplate->templates().first().value, files);
  } else {
    QString msg = createFileList(files, 3);
    if (!msg.isEmpty())
      msg = QStringLiteral("Update ") + msg;
    setMessage(msg);
  }
}

void CommitEditor::setMessage(const QString &message) {
  mMessage->setPlainText(message);
  mMessage->selectAll();
}

QString CommitEditor::message() const { return mMessage->toPlainText(); }

void CommitEditor::setDiff(const git::Diff &diff) {
  mDiff = diff;
  updateButtons(false);

  // Pre-populate commit editor with the merge message.
  QString msg = RepoView::parentView(this)->repo().message();
  if (!msg.isEmpty())
    mMessage->setPlainText(msg);
}

// public slots
void CommitEditor::applyTemplate(const QString &t, const QStringList &files) {

  QString templ = t;

  QString pattern = TemplateButton::filesPosition;
  pattern.replace("{", "\\{");
  pattern.replace("}", "\\}");
  pattern.replace("$", "\\$");
  QRegularExpression re(pattern);
  QRegularExpressionMatch match = re.match(templ);
  int start = -1;
  int offset = 0;
  if (match.hasMatch()) {
    start = match.capturedStart(0);
    int origLength = match.capturedLength(0);
    const auto matchComplete = match.captured(0);
    bool ok;
    const auto number = match.captured(1).toInt(&ok);

    if (ok) {
      const QString filesStr = createFileList(files, number);
      templ.replace(matchComplete, filesStr);
      offset = filesStr.length() - origLength;
    }
  }

  auto index = t.indexOf(TemplateButton::cursorPositionString);
  if (index < 0)
    index = templ.length();
  else if (start > 0 && index > start) {
    // offset, because fileStr has different length than matchComplete
    index += offset;
  }

  templ.replace(TemplateButton::cursorPositionString, "");
  mMessage->setText(templ);
  auto cursor = mMessage->textCursor();
  cursor.setPosition(index);
  mMessage->setTextCursor(cursor);
}

void CommitEditor::applyTemplate(const QString &t) { applyTemplate(t, {}); }

void CommitEditor::updateButtons(bool yieldFocus) {
  RepoView *view = RepoView::parentView(this);
  if (!view || !view->repo().isValid()) {
    mRebaseContinue->setVisible(false);
    mRebaseAbort->setVisible(false);
  } else {
    const bool rebaseOngoing = view->repo().rebaseOngoing();
    mRebaseContinue->setVisible(rebaseOngoing);
    mRebaseAbort->setVisible(rebaseOngoing);
  }

  // TODO: copied from menubar
  bool merging = false;
  QString text = tr("Merge");
  if (view) {
    switch (view->repo().state()) {
      case GIT_REPOSITORY_STATE_MERGE:
        merging = true;
        break;

      case GIT_REPOSITORY_STATE_REVERT:
      case GIT_REPOSITORY_STATE_REVERT_SEQUENCE:
        merging = true;
        text = tr("Revert");
        break;

      case GIT_REPOSITORY_STATE_CHERRYPICK:
      case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE:
        merging = true;
        text = tr("Cherry-pick");
        break;

      case GIT_REPOSITORY_STATE_REBASE:
      case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
      case GIT_REPOSITORY_STATE_REBASE_MERGE:
        text = tr("Rebase");
        break;
    }
  }

  git::Reference head = view ? view->repo().head() : git::Reference();
  git::Branch headBranch = head;

  mMergeAbort->setText(tr("Abort %1").arg(text));
  mMergeAbort->setVisible(headBranch.isValid() && merging);

  if (!mDiff.isValid()) {
    mStage->setEnabled(false);
    mUnstage->setEnabled(false);
    mCommit->setEnabled(false);
    return;
  }

  QStringList files;
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
        files.append(QFileInfo(name).fileName());
        ++partial;
        break;

      case git::Index::Staged:
        files.append(QFileInfo(name).fileName());
        ++staged;
        break;

      case git::Index::Conflicted:
        ++conflicted;
        break;
    }
  }

  if (mPopulate) {
    QSignalBlocker blocker(mMessage);
    (void)blocker;

    setMessage(files);
    if (yieldFocus && !mMessage->toPlainText().isEmpty())
      mMessage->setFocus();
  }

  int total = staged + partial + conflicted;
  mStage->setEnabled(count > staged);
  mUnstage->setEnabled(total);

  // Set status text.
  QString status = tr("Nothing staged");
  if (staged || partial || conflicted) {
    QString fmt = (staged == 1 && count == 1) ? tr("%1 of %2 file staged")
                                              : tr("%1 of %2 files staged");
    QStringList fragments(fmt.arg(staged).arg(count));

    if (partial) {
      QString partialFmt = (partial == 1) ? tr("%1 file partially staged")
                                          : tr("%1 files partially staged");
      fragments.append(partialFmt.arg(partial));
    }

    if (conflicted) {
      QString conflictedFmt = (conflicted == 1) ? tr("%1 unresolved conflict")
                                                : tr("%1 unresolved conflicts");
      fragments.append(conflictedFmt.arg(conflicted));
    } else if (mDiff.isConflicted()) {
      fragments.append(tr("all conflicts resolved"));
    }

    status = fragments.join(", ");
  }

  mStatus->setText(brightText(status));

  // Change commit button text for committing a merge.
  git::Repository repo = RepoView::parentView(this)->repo();

  switch (repo.state()) {
    case GIT_REPOSITORY_STATE_MERGE:
      mCommit->setText(tr("Commit Merge"));
      mCommit->setEnabled(total && !mMessage->document()->isEmpty());
      break;
    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_REBASE_MERGE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
      mCommit->setText(tr("Commit Rebase"));
      mCommit->setEnabled(total && conflicted == 0 &&
                          !mMessage->document()->isEmpty());
      break;
    default:
      mCommit->setText(tr("Commit"));
      mCommit->setEnabled(total && !mMessage->document()->isEmpty());
      break;
  }

  // Update menu actions.
  MenuBar::instance(this)->updateRepository();
}

QTextEdit *CommitEditor::textEdit() const { return mMessage; }

#include "CommitEditor.moc"
