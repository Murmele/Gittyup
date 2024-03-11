#include "HunkWidget.h"
#include "Line.h"
#include "Debug.h"
#include "git/Diff.h"
#include "DiffView.h"
#include "DisclosureButton.h"
#include "EditButton.h"
#include "DiscardButton.h"
#include "app/Application.h"

#include "Editor.h"

#include "ui/RepoView.h"
#include "ui/MenuBar.h"
#include "ui/EditorWindow.h"
#include "ui/BlameEditor.h"

#include <QVBoxLayout>
#include <QTextLayout>
#include <QTextEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QToolButton>
#include <QAbstractItemView>
#include <QTableWidget>
#include <QShortcut>
#include <QHeaderView>

namespace {

bool disclosure = false;

const QString noNewLineAtEndOfFile =
    HunkWidget::tr("No newline at end of file");
} // namespace

_HunkWidget::HunkLabel::HunkLabel(const QString &name, bool submodule,
                                  QWidget *parent)
    : QWidget(parent), mName(name) {}
void _HunkWidget::HunkLabel::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  QFontMetrics fm = fontMetrics();
  QRect rect = fm.boundingRect(0, 0, this->rect().width(), 300, 0, mName);
  painter.drawText(rect, Qt::AlignLeft | Qt::ElideRight, mName);
}

_HunkWidget::Header::Header(const git::Diff &diff, const git::Patch &patch,
                            int index, bool lfs, bool submodule,
                            QWidget *parent)
    : QFrame(parent) {
  setObjectName("HunkHeader");
  mCheck = new QCheckBox(this);
  mCheck->setTristate(true);
  mCheck->setVisible(diff.isStatusDiff() && !submodule &&
                     !patch.isConflicted());

  QString label_string = (index >= 0) ? patch.header(index) : QString();
  label_string = label_string.trimmed().toHtmlEscaped();
  HunkLabel *label = new HunkLabel(label_string, submodule, this);

  if (patch.isConflicted()) {
    mSave = new QToolButton(this);
    mSave->setObjectName("ConflictSave");
    mSave->setText(HunkWidget::tr("Save"));

    mUndo = new QToolButton(this);
    mUndo->setObjectName("ConflictUndo");
    mUndo->setText(HunkWidget::tr("Undo"));
    connect(mUndo, &QToolButton::clicked, [this] {
      mSave->setVisible(false);
      mUndo->setVisible(false);
      mOurs->setEnabled(true);
      mTheirs->setEnabled(true);
    });

    mOurs = new QToolButton(this);
    mOurs->setObjectName("ConflictOurs");
    mOurs->setStyleSheet(
        Application::theme()->diffButtonStyle(Theme::Diff::Ours));
    mOurs->setText(HunkWidget::tr("Use Ours"));
    connect(mOurs, &QToolButton::clicked, [this] {
      mSave->setVisible(true);
      mUndo->setVisible(true);
      mOurs->setEnabled(false);
      mTheirs->setEnabled(false);
    });

    mTheirs = new QToolButton(this);
    mTheirs->setObjectName("ConflictTheirs");
    mTheirs->setStyleSheet(
        Application::theme()->diffButtonStyle(Theme::Diff::Theirs));
    mTheirs->setText(HunkWidget::tr("Use Theirs"));
    connect(mTheirs, &QToolButton::clicked, [this] {
      mSave->setVisible(true);
      mUndo->setVisible(true);
      mOurs->setEnabled(false);
      mTheirs->setEnabled(false);
    });
  }

  EditButton *edit = new EditButton(patch, index, false, lfs, this);
  edit->setToolTip(HunkWidget::tr("Edit Hunk"));

  // Add discard button.
  DiscardButton *discard = nullptr;
  if (diff.isStatusDiff() && !submodule && !patch.isConflicted()) {
    discard = new DiscardButton(this);
    discard->setToolTip(HunkWidget::tr("Discard Hunk"));

    connect(discard, &DiscardButton::clicked, this,
            &_HunkWidget::Header::discard);
  }

  mButton = new DisclosureButton(this);
  mButton->setToolTip(mButton->isChecked() ? HunkWidget::tr("Collapse Hunk")
                                           : HunkWidget::tr("Expand Hunk"));
  connect(mButton, &DisclosureButton::toggled, [this] {
    mButton->setToolTip(mButton->isChecked() ? HunkWidget::tr("Collapse Hunk")
                                             : HunkWidget::tr("Expand Hunk"));
  });
  mButton->setVisible(disclosure);

  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->setContentsMargins(0, 0, 0, 0);
  buttons->setSpacing(4);
  if (mSave && mUndo && mOurs && mTheirs) {
    mSave->setVisible(false);
    mUndo->setVisible(false);
    buttons->addWidget(mSave);
    buttons->addWidget(mUndo);
    buttons->addWidget(mOurs);
    buttons->addWidget(mTheirs);
    buttons->addSpacing(8);
  }

  buttons->addWidget(edit);
  if (discard)
    buttons->addWidget(discard);
  buttons->addWidget(mButton);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->addWidget(mCheck);
  layout->addWidget(label, 1);
  layout->addStretch();
  layout->addLayout(buttons);

  // Collapse on check.
  connect(mCheck, &QCheckBox::clicked, [this](bool staged) {
    mButton->setChecked(!staged);
    if (staged) {
      emit stageStateChanged(Qt::Checked);
    } else {
      emit stageStateChanged(Qt::Unchecked);
    }
  });
}

void _HunkWidget::Header::setCheckState(
    git::Index::StagedState
        state) { // on the checkstate signal will not be reacted
  if (state == git::Index::Staged)
    mCheck->setCheckState(Qt::Checked);
  else if (state == git::Index::Unstaged)
    mCheck->setCheckState(Qt::Unchecked);
  else
    mCheck->setCheckState(Qt::PartiallyChecked);
}

QCheckBox *_HunkWidget::Header::check() const { return mCheck; }

DisclosureButton *_HunkWidget::Header::button() const { return mButton; }

QToolButton *_HunkWidget::Header::saveButton() const { return mSave; }

QToolButton *_HunkWidget::Header::undoButton() const { return mUndo; }

QToolButton *_HunkWidget::Header::oursButton() const { return mOurs; }

QToolButton *_HunkWidget::Header::theirsButton() const { return mTheirs; }

void _HunkWidget::Header::mouseDoubleClickEvent(QMouseEvent *event) {
  if (mButton->isEnabled())
    mButton->toggle();
}

//#############################################################################
//##########     HunkWidget     ###############################################
//#############################################################################

HunkWidget::HunkWidget(DiffView *view, const git::Diff &diff,
                       const git::Patch &patch, const git::Patch &staged,
                       int index, bool lfs, bool submodule, QWidget *parent)
    : QFrame(parent), mView(view), mPatch(patch), mStaged(staged),
      mIndex(index), mLfs(lfs) {
  setObjectName("HunkWidget");
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  mHeader = new _HunkWidget::Header(diff, patch, index, lfs, submodule, this);
  layout->addWidget(mHeader);
  connect(mHeader, &_HunkWidget::Header::discard, this, &HunkWidget::discard);

  mEditor = new Editor(this);
  mEditor->setLexer(patch.name());
  mEditor->setCaretStyle(CARETSTYLE_INVISIBLE);
  mEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  if (index >= 0)
    mEditor->setLineCount(patch.lineCount(index));
  mEditor->setStatusDiff(diff.isStatusDiff());

  connect(mEditor, &TextEditor::updateUi, MenuBar::instance(this),
          &MenuBar::updateCutCopyPaste);
  connect(mEditor, &TextEditor::stageSelectedSignal, this,
          &HunkWidget::stageSelected);
  connect(mEditor, &TextEditor::unstageSelectedSignal, this,
          &HunkWidget::unstageSelected);
  connect(mEditor, &TextEditor::discardSelectedSignal, this,
          &HunkWidget::discardDialog);
  connect(mEditor, &TextEditor::marginClicked, this,
          &HunkWidget::marginClicked);

  // Ensure that text margin reacts to settings changes.
  connect(mEditor, &TextEditor::settingsChanged, [this] {
    int width = mEditor->textWidth(STYLE_LINENUMBER, mEditor->marginText(0));
    mEditor->setMarginWidthN(TextEditor::LineNumbers, width);
  });

  // Darken background when find highlight is active.
  connect(mEditor, &TextEditor::highlightActivated, this,
          &HunkWidget::setDisabled);

  // Disable vertical resize.
  mEditor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  layout->addWidget(mEditor);
  if (disclosure)
    connect(mHeader->button(), &DisclosureButton::toggled, mEditor,
            &TextEditor::setVisible);
  connect(mHeader, &_HunkWidget::Header::stageStateChanged, this,
          &HunkWidget::headerCheckStateChanged);

  // Handle conflict resolution.
  if (QToolButton *save = mHeader->saveButton()) {
    connect(save, &QToolButton::clicked, [this] {
      TextEditor editor;
      git::Repository repo = mPatch.repo();
      QString path = repo.workdir().filePath(mPatch.name());

      {
        // Read file.
        QFile file(path);
        if (file.open(QFile::ReadOnly))
          editor.load(path, repo.decode(file.readAll()));
      }

      if (!editor.length())
        return;

      // Apply resolution.
      for (int i = mPatch.lineCount(mIndex); i >= 0; --i) {
        char origin = mPatch.lineOrigin(mIndex, i);
        auto resolution = mPatch.conflictResolution(mIndex);
        if (origin == GIT_DIFF_LINE_CONTEXT ||
            (origin == 'O' && resolution == git::Patch::Ours) ||
            (origin == 'T' && resolution == git::Patch::Theirs))
          continue;

        int line = mPatch.lineNumber(mIndex, i);
        int pos = editor.positionFromLine(line);
        int length = editor.lineLength(line);
        editor.deleteRange(pos, length);
      }

      // Write file to disk.
      QSaveFile file(path);
      if (!file.open(QFile::WriteOnly))
        return;

      QTextStream out(&file);
      out.setCodec(repo.codec());
      out << editor.text();
      file.commit();

      mPatch.setConflictResolution(mIndex, git::Patch::Unresolved);

      RepoView::parentView(this)->refresh();
    });
  }

  if (QToolButton *undo = mHeader->undoButton()) {
    connect(undo, &QToolButton::clicked, [this] {
      // Invalidate to trigger reload.
      invalidate();
      mPatch.setConflictResolution(mIndex, git::Patch::Unresolved);
    });
  }

  if (QToolButton *ours = mHeader->oursButton()) {
    connect(ours, &QToolButton::clicked, [this] {
      mEditor->markerDeleteAll(TextEditor::Theirs);
      chooseLines(TextEditor::Ours);
      mPatch.setConflictResolution(mIndex, git::Patch::Ours);
    });
  }

  if (QToolButton *theirs = mHeader->theirsButton()) {
    connect(theirs, &QToolButton::clicked, [this] {
      mEditor->markerDeleteAll(TextEditor::Ours);
      chooseLines(TextEditor::Theirs);
      mPatch.setConflictResolution(mIndex, git::Patch::Theirs);
    });
  }

  // Hook up error margin click.
  bool status = diff.isStatusDiff();
  connect(mEditor, &TextEditor::marginClicked,
          [this, status](int pos, int modifier, int margin) {
            if (margin !=
                TextEditor::Margin::ErrorMargin) // Handle only Error margin
              return;
            int line = mEditor->lineFromPosition(pos);
            QList<TextEditor::Diagnostic> diags = mEditor->diagnostics(line);
            if (diags.isEmpty())
              return;

            QTableWidget *table = new QTableWidget(diags.size(), 3);
            table->setWindowFlag(Qt::Popup);
            table->setAttribute(Qt::WA_DeleteOnClose);
            table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

            table->setShowGrid(false);
            table->setSelectionMode(QAbstractItemView::NoSelection);
            table->verticalHeader()->setVisible(false);
            table->horizontalHeader()->setVisible(false);

            QShortcut *esc = new QShortcut(tr("Esc"), table);
            connect(esc, &QShortcut::activated, table, &QTableWidget::close);

            for (int i = 0; i < diags.size(); ++i) {
              const TextEditor::Diagnostic &diag = diags.at(i);

              QStyle::StandardPixmap pixmap;
              switch (diag.kind) {
                case TextEditor::Note:
                  pixmap = QStyle::SP_MessageBoxInformation;
                  break;

                case TextEditor::Warning:
                  pixmap = QStyle::SP_MessageBoxWarning;
                  break;

                case TextEditor::Error:
                  pixmap = QStyle::SP_MessageBoxCritical;
                  break;
              }

              QIcon icon = style()->standardIcon(pixmap);
              QTableWidgetItem *item = new QTableWidgetItem(icon, diag.message);
              item->setToolTip(diag.description);
              table->setItem(i, 0, item);

              // Add fix button. Disable for deletion lines.
              QPushButton *fix = new QPushButton(tr("Fix"));
              bool deletion =
                  (mEditor->markers(line) & (1 << TextEditor::Deletion));
              fix->setEnabled(status && !deletion &&
                              !diag.replacement.isNull());
              connect(fix, &QPushButton::clicked, [this, line, diag, table] {
                // Look up the actual line number from the margin.
                QRegularExpression re("\\s+");
                QStringList numbers = mEditor->marginText(line).split(re);
                if (numbers.size() != 2)
                  return;

                int newLine = numbers.last().toInt() - 1;
                if (newLine < 0)
                  return;

                // Load editor.
                TextEditor editor;
                git::Repository repo = mPatch.repo();
                QString path = repo.workdir().filePath(mPatch.name());

                {
                  // Read file.
                  QFile file(path);
                  if (file.open(QFile::ReadOnly))
                    editor.load(path, repo.decode(file.readAll()));
                }

                if (!editor.length())
                  return;

                // Replace range.
                int pos = editor.positionFromLine(newLine) + diag.range.pos;
                editor.setSelection(pos + diag.range.len, pos);
                editor.replaceSelection(diag.replacement);

                // Write file to disk.
                QSaveFile file(path);
                if (!file.open(QFile::WriteOnly))
                  return;

                QTextStream out(&file);
                out.setCodec(repo.codec());
                out << editor.text();
                file.commit();

                table->hide();
                RepoView::parentView(this)->refresh();
              });

              table->setCellWidget(i, 1, fix);

              // Add edit button.
              QPushButton *edit = new QPushButton(tr("Edit"));
              connect(edit, &QPushButton::clicked, [this, line, diag] {
                // Look up the actual line number from the margin.
                QRegularExpression re("\\s+");
                QStringList numbers = mEditor->marginText(line).split(re);
                if (numbers.size() != 2)
                  return;

                int newLine = numbers.last().toInt() - 1;
                if (newLine < 0)
                  return;

                // Edit the file and select the range.
                RepoView *view = RepoView::parentView(this);
                EditorWindow *window = view->openEditor(mPatch.name(), newLine);
                TextEditor *editor = window->widget()->editor();
                int pos = editor->positionFromLine(newLine) + diag.range.pos;
                editor->setSelection(pos + diag.range.len, pos);
              });

              table->setCellWidget(i, 2, edit);
            }

            table->resizeColumnsToContents();
            table->resize(table->sizeHint());

            QPoint point = mEditor->pointFromPosition(pos);
            point.setY(point.y() + mEditor->textHeight(line));
            table->move(mEditor->mapToGlobal(point));
            table->show();
          });
}

_HunkWidget::Header *HunkWidget::header() const { return mHeader; }

TextEditor *HunkWidget::editor(bool ensureLoaded) {
  if (ensureLoaded)
    load(mStaged);
  return mEditor;
}

void HunkWidget::invalidate() {
  mEditor->setReadOnly(false);
  mEditor->clearAll();
  mLoaded = false;
  update();
}

void HunkWidget::paintEvent(QPaintEvent *event) {
  load(mStaged);
  QFrame::paintEvent(event);
}

void HunkWidget::stageSelected(int startLine, int end, bool emitSignal) {
  for (int i = startLine; i < end; i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Addition |
                1 << TextEditor::Marker::Deletion)) {
      // stage only when not already staged
      if ((mask & 1 << TextEditor::Marker::StagedMarker) == 0) {
        mEditor->markerAdd(i, TextEditor::StagedMarker);
        mEditor->markerDelete(i, TextEditor::UnstagedMarker);
      }
    }
  }

  mStagedStateLoaded = false;
  if (!mLoading && emitSignal)
    emit stageStateChanged(stageState());
}
void HunkWidget::unstageSelected(int startLine, int end, bool emitSignal) {
  for (int i = startLine; i < end; i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Addition |
                1 << TextEditor::Marker::Deletion)) {
      if ((mask & 1 << TextEditor::Marker::UnstagedMarker) == 0) {
        mEditor->markerAdd(i, TextEditor::UnstagedMarker);
        mEditor->markerDelete(i, TextEditor::StagedMarker);
      }
    }
  }

  mStagedStateLoaded = false;
  if (!mLoading && emitSignal)
    emit stageStateChanged(stageState());
}

void HunkWidget::discardDialog(int startLine, int end) {
  QString name = mPatch.name();
  QString title = HunkWidget::tr("Discard selected lines?");
  QString text =
      mPatch.isUntracked()
          ? HunkWidget::tr("Are you sure you want to remove '%1'?").arg(name)
          : HunkWidget::tr("Are you sure you want to discard the "
                           "changes in hunk from line %1 to %2 in '%3'?")
                .arg(startLine)
                .arg(end - 1)
                .arg(name);

  QMessageBox *dialog = new QMessageBox(QMessageBox::Warning, title, text,
                                        QMessageBox::Cancel, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setInformativeText(HunkWidget::tr("This action cannot be undone."));

  QPushButton *discard = dialog->addButton(
      HunkWidget::tr("Discard selected lines"), QMessageBox::AcceptRole);
  connect(discard, &QPushButton::clicked,
          [this, startLine, end] { discardSelected(startLine, end); });

  dialog->open();
}

void HunkWidget::discardSelected(int startLine, int end) {
  for (int i = startLine; i < end; i++) {
    int mask = mEditor->markers(i);
    if (mask &
        (1 << TextEditor::Marker::Addition | 1 << TextEditor::Marker::Deletion))
      mEditor->markerAdd(i, TextEditor::DiscardMarker);
  }
  emit discardSignal();
}

void HunkWidget::headerCheckStateChanged(int state) {

  assert(state != Qt::PartiallyChecked); // makes no sense, that the user can
                                         // select partially selected
  git::Index::StagedState stageState;
  if (state == Qt::Checked)
    stageState = git::Index::StagedState::Staged;
  else
    stageState = git::Index::StagedState::Unstaged;

  // must be done, because the stage state of the hole file is calculated
  // from the staged lines in the editor
  setStageState(stageState);

  stageStateChanged(stageState);
}

void HunkWidget::setStageState(git::Index::StagedState state) {

  if (state == git::Index::StagedState::Staged ||
      state == git::Index::StagedState::Unstaged) {
    mHeader->setCheckState(state);
    // update the line markers
    bool staged = state == git::Index::StagedState::Staged ? true : false;
    int lineCount = mEditor->lineCount();
    int count = 0;
    for (int i = 0; i < lineCount; i++) {
      int mask = mEditor->markers(i);
      // if a line was not added or deleted, it cannot be staged so ignore all
      // of them
      if (mask & (1 << TextEditor::Marker::Addition |
                  1 << TextEditor::Marker::Deletion)) {
        setStaged(i, staged, false);
        count++;
      }
    }
  }
  mStagedStage = state;
}

void HunkWidget::setStaged(bool staged) {
  if (staged)
    setStageState(git::Index::StagedState::Staged);
  else
    setStageState(git::Index::StagedState::Unstaged);
}

void HunkWidget::discard() {
  int count = mEditor->lineCount();
  discardDialog(0, count);
}

void HunkWidget::setStaged(int lidx, bool staged, bool emitSignal) {
  int markers = mEditor->markers(lidx);
  if (!(markers & (1 << TextEditor::Marker::Addition |
                   1 << TextEditor::Marker::Deletion)))
    return;

  if (staged && ((markers & (1 << TextEditor::Marker::StagedMarker)) > 0))
    return;

  if (staged)
    stageSelected(lidx, lidx + 1, emitSignal);
  else if (!staged)
    unstageSelected(lidx, lidx + 1, emitSignal);
  mLoaded = true;
}

void HunkWidget::marginClicked(int pos, int modifier, int margin) {
  if (margin != TextEditor::Margin::Staged)
    return;

  int lidx = mEditor->lineFromPosition(pos);

  int markers = mEditor->markers(lidx);

  if (!(markers & (1 << TextEditor::Marker::Addition |
                   1 << TextEditor::Marker::Deletion)))
    return;

  if (markers & 1 << TextEditor::Marker::StagedMarker)
    setStaged(lidx, false);
  else
    setStaged(lidx, true);
}

int HunkWidget::tokenEndPosition(int pos) const {
  int length = mEditor->length();
  char ch = mEditor->charAt(pos);
  QByteArray wordChars = mEditor->wordChars();
  if (wordChars.contains(ch)) {
    do {
      ch = mEditor->charAt(++pos);
    } while (pos < length && wordChars.contains(ch));
    return pos;
  }

  QByteArray spaceChars = mEditor->whitespaceChars();
  if (spaceChars.contains(ch)) {
    do {
      ch = mEditor->charAt(++pos);
    } while (pos < length && spaceChars.contains(ch));
    return pos;
  }

  return pos + 1;
}

QList<HunkWidget::Token> HunkWidget::tokens(int line) const {
  QList<Token> tokens;

  int end = mEditor->lineEndPosition(line);
  int pos = mEditor->positionFromLine(line);
  while (pos < end) {
    int wordEnd = tokenEndPosition(pos);
    tokens.append({pos, mEditor->textRange(pos, wordEnd)});
    pos = wordEnd;
  }

  // Add sentinel.
  tokens.append({pos, ""});

  return tokens;
}

QByteArray HunkWidget::tokenBuffer(const QList<HunkWidget::Token> &tokens) {
  QByteArrayList list;
  foreach (const Token &token, tokens)
    list.append(token.text);
  return list.join('\n');
}

void HunkWidget::load() { load(mStaged, true); }

void HunkWidget::load(git::Patch &staged, bool force) {
  if (!force && mLoaded)
    return;

  mLoaded = true;
  mLoading = true;
  mStagedStateLoaded = false;

  mStaged = staged;

  mEditor->markerDeleteAll(-1); // delete all markers on each line

  // Load entire file.
  git::Repository repo = mPatch.repo();
  if (mIndex < 0) {
    QString name = mPatch.name();
    QFile dev(repo.workdir().filePath(name));
    if (dev.open(QFile::ReadOnly)) {
      mEditor->load(name, repo.decode(dev.readAll()));

      int count = mEditor->lineCount();
      QByteArray lines = QByteArray::number(count);
      int width = mEditor->textWidth(STYLE_LINENUMBER, lines.constData());
      int marginWidth = (mEditor->length() > 0) ? width + 8 : 0;
      mEditor->setMarginWidthN(TextEditor::LineNumber, marginWidth);
    }

    // Disallow editing.
    mEditor->setReadOnly(true);

    return;
  }

  if (mLfs) {
    // TODO: show pdfs, if it is a pdf!
    return;
  }

  Debug("Diff Header:");
  for (int i = 0; i < mPatch.count(); i++) {
    Debug(mPatch.header(i));
  }

  Debug("Staged Header:");
  for (int i = 0; i < mStaged.count(); i++) {
    Debug(mStaged.header(i));
  }

  // Load hunk.
  QList<Line> lines;
  QByteArray content;

  // Create content for the editor
  int patchCount = mPatch.lineCount(mIndex);
  for (int lidx = 0; lidx < patchCount; ++lidx) {
    char origin = mPatch.lineOrigin(mIndex, lidx);
    if (origin == GIT_DIFF_LINE_CONTEXT_EOFNL ||
        origin == GIT_DIFF_LINE_ADD_EOFNL ||
        origin == GIT_DIFF_LINE_DEL_EOFNL) {
      Q_ASSERT(!lines.isEmpty());
      lines.last().setNewline(false);
      content += '\n';
    } else
      content += mPatch.lineContent(mIndex, lidx);

    int oldLine = mPatch.lineNumber(mIndex, lidx, git::Diff::OldFile);
    int newLine = mPatch.lineNumber(mIndex, lidx, git::Diff::NewFile);
    lines << Line(origin, oldLine, newLine);
  }

  // Trim final line end.
  // Don't do that because then the editor line
  // does not match anymore the blob line.
  // this is a problem when copying the hunk
  // back into the blob as done in the discardHunk()
  // method in FileWidget
  //  if (content.endsWith('\n'))
  //    content.chop(1);
  //  if (content.endsWith('\r'))
  //    content.chop(1);

  // Add text.
  mEditor->setText(repo.decode(content));

  // Calculate margin width.
  int width = 0;
  int conflictWidth = 0;
  foreach (const Line &line, lines) {
    int oldWidth = line.oldLine().length();
    int newWidth = line.newLine().length();
    width = qMax(width, oldWidth + newWidth + 1);
    conflictWidth = qMax(conflictWidth, oldWidth);
  }

  // Get comments for this file.
  Account::FileComments comments = mView->comments().files.value(mPatch.name());

  setEditorLineInfos(lines, comments, width);

  // Set margin width.
  QByteArray text(mPatch.isConflicted() ? conflictWidth : width, ' ');
  int margin = mEditor->textWidth(STYLE_DEFAULT, text);
  if (margin > mEditor->marginWidthN(TextEditor::LineNumbers))
    mEditor->setMarginWidthN(TextEditor::LineNumbers, margin);

  // Disallow editing.
  mEditor->setReadOnly(true);

  // Restore resolved conflicts.
  if (mPatch.isConflicted()) {
    switch (mPatch.conflictResolution(mIndex)) {
      case git::Patch::Ours:
        mHeader->oursButton()->click();
        break;

      case git::Patch::Theirs:
        mHeader->theirsButton()->click();
        break;

      case git::Patch::Unresolved:
        break;
    }
  }

  // Execute hunk plugins.
  foreach (PluginRef plugin, mView->plugins()) {
    if (plugin->isValid() && plugin->isEnabled())
      plugin->hunk(mEditor);
  }

  mEditor->updateGeometry();

  mLoading = false;
  mHeader->setCheckState(stageState());
}

void HunkWidget::setEditorLineInfos(QList<Line> &lines,
                                    Account::FileComments &comments,
                                    int width) {
  Debug("Patch lines:" << lines.count());
  for (int i = 0; i < lines.count(); i++) {
    Debug(i << ") " << lines[i].print()
            << "Content: " << mPatch.lineContent(mIndex, i));
  }

  Debug("Staged linesStaged:" << mStaged.count());
  for (int i = 0; i < mStaged.count(); i++) {
    Debug("Staged patch No. " << i);
    for (int lidx = 0; lidx < mStaged.lineCount(i); lidx++) {
      auto origin = mStaged.lineOrigin(i, lidx);
      int oldLineStaged = mStaged.lineNumber(i, lidx, git::Diff::OldFile);
      int newLineStaged = mStaged.lineNumber(i, lidx, git::Diff::NewFile);
      auto line = Line(origin, oldLineStaged, newLineStaged);
      Debug(lidx << ") " << line.print() << mStaged.lineContent(i, lidx));
    }
  }

  //	New file line number change for different line origins
  //	| diff HEAD         | diff –cached |   |
  //	|-------------------|--------------|---|
  //	| no change         | +            | + |
  //	| unstaged addition | +            | / |
  //	| staged addition   | +            | + |
  //	| unstaged deletion | /            | + |
  //	| staged deletion   | /            | / |

  // Patch index change for different line origins
  //	| diff HEAD         | diff –cached |   |
  //	|-------------------|--------------|---|
  //	| no change         | +            | + |
  //	| unstaged addition | +            | / |
  //	| staged addition   | +            | + |
  //	| unstaged deletion | +            | / |
  //	| staged deletion   | +            | + |

  const int count = lines.size();
  int marker = -1;
  int additions = 0, deletions = 0;
  int additions_tot = 0, deletions_tot = 0;
  int stagedAdditions = 0, stagedDeletions = 0;
  bool staged = false;
  int current_staged_index = -1;
  int current_staged_line_idx = 0;

  // Find the first staged hunk which is within this patch
  if (!mPatch.isConflicted()) {
    auto patch_header_struct = mPatch.header_struct(mIndex);
    for (int i = 0; i < mStaged.count(); i++) {
      if (patch_header_struct->old_start >
          mStaged.header_struct(i)->old_start +
              mStaged.header_struct(i)->old_lines)
        continue;
      else {
        current_staged_index = i;
        break;
      }
    }
  }

  // Set all editor markers, like green background for adding, red background
  // for deletion Blue background for OURS or magenta background for Theirs
  // Setting also the staged symbol
  int skipPatchLines = 0;
  bool first_staged_patch_match = false;
  for (int lidx = 0; lidx < count; ++lidx) {
    const Line &line = lines.at(lidx);
    Debug("Line Content: " << mPatch.lineContent(mIndex, lidx));
    const auto lineOrigin = line.origin();
    marker = -1;
    staged = false;
    if (lineOrigin != GIT_DIFF_LINE_CONTEXT_EOFNL &&
        lineOrigin != GIT_DIFF_LINE_ADD_EOFNL &&
        lineOrigin != GIT_DIFF_LINE_DEL_EOFNL)
      createMarkersAndLineNumbers(line, lidx - skipPatchLines, comments, width);

    // Find matching lines if the first line of the staged patch is somewhere in
    // the middle of the complete patch Switch to next staged patch if the line
    // number is higher than the current staged patch contains
    if (!mPatch.isConflicted() && current_staged_index >= 0) {
      auto staged_header_struct_next =
          mStaged.header_struct(current_staged_index + 1);
      if (!first_staged_patch_match) {
        if ((lineOrigin == GIT_DIFF_LINE_CONTEXT ||
             lineOrigin == GIT_DIFF_LINE_DELETION) &&
            mPatch.lineNumber(mIndex, lidx, git::Diff::OldFile) ==
                mStaged.lineNumber(current_staged_index, 0,
                                   git::Diff::OldFile)) {
          first_staged_patch_match = true;
          current_staged_line_idx = 0;
        } else if (lineOrigin == GIT_DIFF_LINE_ADDITION &&
                   /* If the staged line is not an addition, it cannot be that
                      it will be matched */
                   mStaged.lineOrigin(current_staged_index, 0) ==
                       GIT_DIFF_LINE_ADDITION &&
                   mPatch.lineNumber(mIndex, lidx, git::Diff::NewFile) ==
                       mStaged.lineNumber(current_staged_index, 0,
                                          git::Diff::NewFile))
          first_staged_patch_match = true;
        current_staged_line_idx = 0;
      } else if (staged_header_struct_next &&
                 mPatch.lineNumber(mIndex, lidx, git::Diff::OldFile) ==
                     staged_header_struct_next->old_start) {
        // Align staged patch with total patch
        current_staged_index++;
        assert(mPatch.lineContent(mIndex, lidx) ==
               mStaged.lineContent(current_staged_index, 0));
        current_staged_line_idx = 0;
      }
    }

    switch (lineOrigin) {
      case GIT_DIFF_LINE_CONTEXT:
        marker = TextEditor::Context;
        additions = 0;
        deletions = 0;
        current_staged_line_idx++;
        break;

      case GIT_DIFF_LINE_ADDITION: {
        marker = TextEditor::Addition;
        additions++;
        // Find matching lines
        if (lidx + 1 >= count ||
            mPatch.lineOrigin(mIndex, lidx + 1) !=
                GIT_DIFF_LINE_ADDITION) { // end of file, or the last addition
                                          // line
          // The heuristic is that matching blocks have
          // the same number of additions as deletions.
          if (additions == deletions) {
            for (int i = 0; i < additions; ++i) {
              int current = lidx - i;
              int match = current - additions;
              lines[current].setMatchingLine(match);
              lines[match].setMatchingLine(current);
            }
          }
          // Only for performance reason, because the above loop
          // iterates over all additions
          additions = 0;
          deletions = 0;
        }
        // Check if staged
        if (!mPatch.isConflicted() && mStaged.count() > 0 &&
            current_staged_index >= 0 &&
            current_staged_line_idx < mStaged.lineCount(current_staged_index)) {
          auto line_origin =
              mStaged.lineOrigin(current_staged_index, current_staged_line_idx);
          // TODO: try to avoid comparing strings and use line number comparsion
          // like for deletion
          if (line_origin == '+' &&
              mStaged.lineContent(current_staged_index,
                                  current_staged_line_idx) ==
                  mPatch.lineContent(mIndex, lidx)) {
            stagedAdditions++;
            current_staged_line_idx++;
            staged = true;
          }
        }
        additions_tot++;
        break;
      }
      case GIT_DIFF_LINE_DELETION: {
        marker = TextEditor::Deletion;
        deletions++;
        deletions_tot++;

        // Check if staged
        if (!mPatch.isConflicted() && mStaged.count() > 0 &&
            current_staged_index >= 0 &&
            current_staged_line_idx < mStaged.lineCount(current_staged_index)) {
          const auto line_origin =
              mStaged.lineOrigin(current_staged_index, current_staged_line_idx);
          const auto staged_old_file =
              mStaged.lineNumber(current_staged_index, current_staged_line_idx,
                                 git::Diff::OldFile);
          const auto patch_old_file =
              mPatch.lineNumber(mIndex, lidx, git::Diff::OldFile);
          const auto stagedContent = mStaged.lineContent(
              current_staged_index, current_staged_line_idx);
          const auto patchContent = mPatch.lineContent(mIndex, lidx);
          if (line_origin == '-' && staged_old_file == patch_old_file) {
            assert(mStaged.lineContent(current_staged_index,
                                       current_staged_line_idx) ==
                   mPatch.lineContent(mIndex, lidx));
            stagedDeletions++;
            staged = true;
          }
        }
        current_staged_line_idx++; // must be in staged and in the unstaged case
        break;
      }
      case 'O':
        marker = TextEditor::Ours;
        break;

      case 'T':
        marker = TextEditor::Theirs;
        break;
      case GIT_DIFF_LINE_CONTEXT_EOFNL:
      case GIT_DIFF_LINE_ADD_EOFNL:
      case GIT_DIFF_LINE_DEL_EOFNL: {
        skipPatchLines++;
        current_staged_line_idx++;
        continue; // skip setting marker, because for this patchline no
                  // editorline exists
      }
    }

    // Add marker.
    if (marker >= 0)
      mEditor->markerAdd(lidx - skipPatchLines, marker);
    setStaged(lidx - skipPatchLines, staged);
  }

  // Diff matching lines. Highlight changes within the line!
  for (int lidx = 0; lidx < count; ++lidx) {
    const Line &line = lines.at(lidx);
    int matchingLine = line.matchingLine();
    if (line.origin() == GIT_DIFF_LINE_DELETION && matchingLine >= 0) {
      // Split lines into tokens and diff corresponding tokens.
      QList<Token> oldTokens = tokens(lidx);
      QList<Token> newTokens = tokens(matchingLine);
      QByteArray oldBuffer = tokenBuffer(oldTokens);
      QByteArray newBuffer = tokenBuffer(newTokens);
      git::Patch patch = git::Patch::fromBuffers(oldBuffer, newBuffer);
      for (int pidx = 0; pidx < patch.count(); ++pidx) {
        // Find the boundary between additions and deletions.
        int index;
        int count = patch.lineCount(pidx);
        for (index = 0; index < count; ++index) {
          if (patch.lineOrigin(pidx, index) == GIT_DIFF_LINE_ADDITION)
            break;
        }

        // Map differences onto the deletion line.
        if (index > 0) {
          int first = patch.lineNumber(pidx, 0, git::Diff::OldFile) - 1;
          int last = patch.lineNumber(pidx, index - 1, git::Diff::OldFile);

          int size = oldTokens.size();
          if (first >= 0 && first < size && last >= 0 && last < size) {
            int pos = oldTokens.at(first).pos;
            mEditor->setIndicatorCurrent(TextEditor::WordDeletion);
            mEditor->indicatorFillRange(pos, oldTokens.at(last).pos - pos);
          }
        }

        // Map differences onto the addition line.
        if (index < count) {
          int first = patch.lineNumber(pidx, index, git::Diff::NewFile) - 1;
          int last = patch.lineNumber(pidx, count - 1, git::Diff::NewFile);

          int size = newTokens.size();
          if (first >= 0 && first < size && last >= 0 && last < size) {
            int pos = newTokens.at(first).pos;
            mEditor->setIndicatorCurrent(TextEditor::WordAddition);
            mEditor->indicatorFillRange(pos, newTokens.at(last).pos - pos);
          }
        }
      }
    }
  }
}

void HunkWidget::createMarkersAndLineNumbers(const Line &line, int lidx,
                                             Account::FileComments &comments,
                                             int width) const {
  QByteArray oldLine = line.oldLine();
  QByteArray newLine = line.newLine();
  int spaces = width - (oldLine.length() + newLine.length());
  QByteArray text = oldLine + QByteArray(spaces, ' ') + newLine;
  mEditor->marginSetText(lidx, text);
  mEditor->marginSetStyle(lidx, STYLE_LINENUMBER);

  // Build annotations.
  QList<Annotation> annotations;
  if (!line.newline()) {
    QString text = noNewLineAtEndOfFile;
    QByteArray styles =
        QByteArray(text.toUtf8().size(), TextEditor::EofNewline);
    annotations.append({text, styles});
  }

  auto it = comments.constFind(lidx);
  if (it != comments.constEnd()) {
    QString whitespace(DiffViewStyle::kIndent, ' ');
    QFont font = mEditor->styleFont(TextEditor::CommentBody);
    int margin =
        QFontMetrics(font).horizontalAdvance(' ') * DiffViewStyle::kIndent * 2;
    int width = mEditor->textRectangle().width() - margin - 50;

    foreach (const QDateTime &key, it->keys()) {
      QStringList paragraphs;
      Account::Comment comment = it->value(key);
      foreach (const QString &paragraph, comment.body.split('\n')) {
        if (paragraph.isEmpty()) {
          paragraphs.append(QString());
          continue;
        }

        QStringList lines;
        QTextLayout layout(paragraph, font);
        layout.beginLayout();

        forever {
          QTextLine line = layout.createLine();
          if (!line.isValid())
            break;

          line.setLineWidth(width);
          QString text = paragraph.mid(line.textStart(), line.textLength());
          lines.append(whitespace + text.trimmed() + whitespace);
        }

        layout.endLayout();
        paragraphs.append(lines.join('\n'));
      }

      QString author = comment.author;
      QString time = QLocale().toString(key, QLocale::LongFormat);
      QString body = paragraphs.join('\n');
      QString text = author + ' ' + time + '\n' + body;
      QByteArray styles =
          QByteArray(author.toUtf8().size() + 1, TextEditor::CommentAuthor) +
          QByteArray(time.toUtf8().size() + 1, TextEditor::CommentTimestamp) +
          QByteArray(body.toUtf8().size(), TextEditor::CommentBody);
      annotations.append({text, styles});
    }
  }

  QString atnText;
  QByteArray atnStyles;
  foreach (const Annotation &annotation, annotations) {
    if (!atnText.isEmpty()) {
      atnText.append("\n\n");
      atnStyles.append(QByteArray(2, TextEditor::CommentBody));
    }

    atnText.append(annotation.text);
    atnStyles.append(annotation.styles);
  }

  // Set annotations.
  if (!atnText.isEmpty()) {
    mEditor->annotationSetText(lidx, atnText);
    mEditor->annotationSetStyles(lidx, atnStyles);
    mEditor->annotationSetVisible(ANNOTATION_STANDARD);
  }
}

QByteArray HunkWidget::hunk() const {
  QByteArray ar;
  int lineCount = mEditor->lineCount();
  bool appended = false;
  for (int i = 0; i < lineCount; i++) {
    int mask = mEditor->markers(i);
    if (mask & 1 << TextEditor::Marker::Addition) {
      if (!(mask & 1 << TextEditor::Marker::DiscardMarker)) {
        ar.append(mEditor->line(i).toUtf8());
        appended = true;
      }
    } else if (mask & 1 << TextEditor::Marker::Deletion) {
      if (mask & 1 << TextEditor::Marker::DiscardMarker) {
        // with a discard, a deletion becomes reverted
        // and the line is still present
        ar.append(mEditor->line(i).toUtf8());
        appended = true;
      }
    } else {
      ar.append(mEditor->line(i).toUtf8());
      appended = true;
    }

    if (appended && mEditor->annotationLines(i) > 0 &&
        mEditor->annotationText(i) == noNewLineAtEndOfFile) {
      ar.remove(ar.length() - 1, 1); // remove new line
    }
  }

  return ar;
}

QByteArray HunkWidget::apply() {

  load(mStaged);
  QByteArray ar;
  int lineCount = mEditor->lineCount();
  for (int i = 0; i < lineCount; i++) {
    bool appended = false;
    int mask = mEditor->markers(i);
    if (mask & 1 << TextEditor::Marker::Addition) {
      if (mask & 1 << TextEditor::Marker::StagedMarker) {
        ar.append(mEditor->line(i).toUtf8());
        appended = true;
      }
    } else if (mask & 1 << TextEditor::Marker::Deletion) {
      if (!(mask & 1 << TextEditor::Marker::StagedMarker)) {
        ar.append(mEditor->line(i).toUtf8());
        appended = true;
      }
    } else {
      ar.append(mEditor->line(i).toUtf8());
      appended = true;
    }

    if (appended && mEditor->annotationLines(i) > 0 &&
        mEditor->annotationText(i) == noNewLineAtEndOfFile) {
      ar.remove(ar.length() - 1, 1); // remove new line
    }
  }

  return ar;
}

git::Index::StagedState HunkWidget::stageState() {
  if (mStagedStateLoaded)
    return mStagedStage;

  int lineCount = mEditor->lineCount();
  int staged = 0;
  int diffLines = 0;
  for (int i = 0; i < lineCount; i++) {
    int mask = mEditor->markers(i);
    if (!(mask & (1 << TextEditor::Marker::Addition |
                  1 << TextEditor::Marker::Deletion)))
      continue;

    diffLines++;
    if (mask & 1 << TextEditor::Marker::StagedMarker) {
      staged++;
      if (staged < diffLines)
        break; // No need to check more, because it is already clear that it is
               // partially staged
    }
  }

  if (!staged)
    mStagedStage = git::Index::Unstaged;
  else if (staged == diffLines)
    mStagedStage = git::Index::Staged;
  else
    mStagedStage = git::Index::PartiallyStaged;

  mStagedStateLoaded = true;

  return mStagedStage;
}

void HunkWidget::chooseLines(TextEditor::Marker kind) {
  // Edit hunk.
  mEditor->setReadOnly(false);
  int mask = ((1 << TextEditor::Context) | (1 << kind));
  for (int i = mEditor->lineCount() - 1; i >= 0; --i) {
    if (!(mask & mEditor->markers(i))) {
      int pos = mEditor->positionFromLine(i);
      int length = mEditor->lineLength(i);
      mEditor->deleteRange(pos, length);
    }
  }

  mEditor->setReadOnly(true);
}
