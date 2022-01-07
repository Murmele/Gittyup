#include "HunkWidget.h"
#include "Line.h"
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
    QString buttonStyle(Theme::Diff role)
    {
      Theme *theme = Application::theme();
      QColor color = theme->diff(role);
      QString pressed = color.darker(115).name();
      return DiffViewStyle::kButtonStyleFmt.arg(color.name(), pressed);
    }

    bool disclosure = false;
}

_HunkWidget::Header::Header(
  const git::Diff &diff,
  const git::Patch &patch,
  int index,
  bool lfs,
  bool submodule,
  QWidget *parent)
  : QFrame(parent)
{
  setObjectName("HunkHeader");
  mCheck = new QCheckBox(this);
  mCheck->setTristate(true);
  mCheck->setVisible(diff.isStatusDiff() && !submodule && !patch.isConflicted());

  QString header = (index >= 0) ? patch.header(index) : QString();
  QString escaped = header.trimmed().toHtmlEscaped();
  QLabel *label = new QLabel(DiffViewStyle::kHunkFmt.arg(escaped), this);

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
    mOurs->setStyleSheet(buttonStyle(Theme::Diff::Ours));
    mOurs->setText(HunkWidget::tr("Use Ours"));
    connect(mOurs, &QToolButton::clicked, [this] {
      mSave->setVisible(true);
      mUndo->setVisible(true);
      mOurs->setEnabled(false);
      mTheirs->setEnabled(false);
    });

    mTheirs = new QToolButton(this);
    mTheirs->setObjectName("ConflictTheirs");
    mTheirs->setStyleSheet(buttonStyle(Theme::Diff::Theirs));
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

    connect(discard, &DiscardButton::clicked, this, &_HunkWidget::Header::discard);
  }

  mButton = new DisclosureButton(this);
  mButton->setToolTip(
    mButton->isChecked() ? HunkWidget::tr("Collapse Hunk") : HunkWidget::tr("Expand Hunk"));
  connect(mButton, &DisclosureButton::toggled, [this] {
    mButton->setToolTip(
      mButton->isChecked() ? HunkWidget::tr("Collapse Hunk") : HunkWidget::tr("Expand Hunk"));
  });
  mButton->setVisible(disclosure);

  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->setContentsMargins(0,0,0,0);
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
  layout->setContentsMargins(4,4,4,4);
  layout->addWidget(mCheck);
  layout->addWidget(label);
  layout->addStretch();
  layout->addLayout(buttons);
}

void _HunkWidget::Header::setStageState(git::Index::StagedState stageState)
{
  if (stageState == git::Index::Staged)
    mCheck->setCheckState(Qt::Checked);
  else if (stageState == git::Index::Unstaged)
    mCheck->setCheckState(Qt::Unchecked);
  else
    mCheck->setCheckState(Qt::PartiallyChecked);
}

QCheckBox *_HunkWidget::Header::check() const
{
    return mCheck;
}

DisclosureButton* _HunkWidget::Header::button() const
{
    return mButton;
}

QToolButton *_HunkWidget::Header::saveButton() const
{
    return mSave;
}

QToolButton *_HunkWidget::Header::undoButton() const
{
    return mUndo;
}

QToolButton *_HunkWidget::Header::oursButton() const
{
    return mOurs;
}

QToolButton *_HunkWidget::Header::theirsButton() const
{
    return mTheirs;
}

void _HunkWidget::Header::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (mButton->isEnabled())
    mButton->toggle();
}

//#############################################################################
//##########     HunkWidget     ###############################################
//#############################################################################

HunkWidget::HunkWidget(
  DiffView *view,
  const git::Diff &diff,
  const git::Patch &patch,
  const git::Patch &staged,
  int index,
  bool lfs,
  bool submodule,
  QWidget *parent)
  : QFrame(parent), mView(view), mPatch(patch), mStaged(staged), mIndex(index), mLfs(lfs)
{
  setObjectName("HunkWidget");
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(0);

  mHeader = new _HunkWidget::Header(diff, patch, index, lfs, submodule, this);
  layout->addWidget(mHeader);

  mEditor = new Editor(this);
  mEditor->setLexer(patch.name());
  mEditor->setCaretStyle(CARETSTYLE_INVISIBLE);
  mEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  if (index >= 0)
    mEditor->setLineCount(patch.lineCount(index));
  mEditor->setStatusDiff(diff.isStatusDiff());

  connect(mEditor, &TextEditor::updateUi,
          MenuBar::instance(this), &MenuBar::updateCutCopyPaste);
  connect(mEditor, &TextEditor::stageSelectedSignal, this, &HunkWidget::stageSelected);
  connect(mEditor, &TextEditor::unstageSelectedSignal, this, &HunkWidget::unstageSelected);
  connect(mEditor, &TextEditor::discardSelectedSignal, this, &HunkWidget::discardDialog);
  connect(mEditor, &TextEditor::marginClicked, this, &HunkWidget::marginClicked);

  // Ensure that text margin reacts to settings changes.
  connect(mEditor, &TextEditor::settingsChanged, [this] {
    int width = mEditor->textWidth(STYLE_LINENUMBER, mEditor->marginText(0));
    mEditor->setMarginWidthN(TextEditor::LineNumbers, width);
  });

  // Darken background when find highlight is active.
  connect(mEditor, &TextEditor::highlightActivated,
          this, &HunkWidget::setDisabled);

  // Disable vertical resize.
  mEditor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  layout->addWidget(mEditor);
  if (disclosure)
    connect(mHeader->button(), &DisclosureButton::toggled, mEditor, &TextEditor::setVisible);

  // Respond to check changes
  connect(mHeader->check(), &QCheckBox::clicked, this, [this](bool checked) {
    git::Index::StagedState stageState = checked ? git::Index::Staged :
                                                   git::Index::Unstaged;
    updateStageState(stageState);
    emit stageStateChanged(stageState);
  });

  // Respond to discard signal
  connect(mHeader, &_HunkWidget::Header::discard, this, [this] {
    discardDialog(0, mEditor->lineCount());
  });

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
  connect(mEditor, &TextEditor::marginClicked, [this, status](int pos, int modifier, int margin) {
        if (margin != TextEditor::Margin::ErrorMargin) // Handle only Error margin
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
      bool deletion = (mEditor->markers(line) & (1 << TextEditor::Deletion));
      fix->setEnabled(status && !deletion && !diag.replacement.isNull());
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

_HunkWidget::Header* HunkWidget::header() const
{
  return mHeader;
}

TextEditor* HunkWidget::editor(bool ensureLoaded)
{
  if (ensureLoaded)
    load(mStaged, true);

  return mEditor;
}

void HunkWidget::invalidate()
{
  mEditor->setReadOnly(false);
  mEditor->clearAll();
  mLoaded = false;
  update();
}

void HunkWidget::paintEvent(QPaintEvent *event)
{
  load(mStaged);
  QFrame::paintEvent(event);
}

void HunkWidget::stageSelected(int startLine, int end)
{
  for (int i = startLine; i < end; i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Addition | 1 << TextEditor::Marker::Deletion)) {
      if (!(mask & (1 << TextEditor::StagedMarker))) {
        mEditor->markerAdd(i, TextEditor::StagedMarker);
        if (mask & (1 << TextEditor::Marker::Addition))
          mStagedAdditions++;
        else
          mStagedDeletions++;
      }
    }
  }

  // Change index
  emit stageStateChanged(stageState());
}

void HunkWidget::unstageSelected(int startLine, int end)
{
  for (int i = startLine; i < end; i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Addition | 1 << TextEditor::Marker::Deletion)) {
      if (mask & (1 << TextEditor::StagedMarker)) {
        mEditor->markerDelete(i, TextEditor::StagedMarker);
        if (mask & (1 << TextEditor::Marker::Addition))
          mStagedAdditions--;
        else
          mStagedDeletions--;
      }
    }
  }

  // Change index
  emit stageStateChanged(stageState());
}

void HunkWidget::discardDialog(int startLine, int end) {
  QString name = mPatch.name();

  // Find additions, deletions and line numbers
  int additions = 0;
  int deletions = 0;
  int firstLine = 0;
  int lastLine = 0;
  for (int i = startLine; i < end; i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Addition)) {
      additions++;

      int newNumber = mPatch.lineNumber(mIndex, i, git::Diff::NewFile);
      int oldNumber = mPatch.lineNumber(mIndex, i, git::Diff::OldFile);
      if (firstLine == 0)
        firstLine = oldNumber > newNumber ? oldNumber : newNumber;

      lastLine = oldNumber > newNumber ? oldNumber : newNumber;
    }
    if (mask & (1 << TextEditor::Marker::Deletion)) {
      deletions++;

      int newNumber = mPatch.lineNumber(mIndex, i, git::Diff::NewFile);
      int oldNumber = mPatch.lineNumber(mIndex, i, git::Diff::OldFile);
      if (firstLine == 0)
        firstLine = oldNumber > newNumber ? oldNumber : newNumber;

      lastLine = oldNumber > newNumber ? oldNumber : newNumber;
    }
  }

  QString title = HunkWidget::tr("Discard selected lines?");
  QString additionText = additions ? additions > 1 ? HunkWidget::tr("%1 lines").arg(additions) :
                                                     HunkWidget::tr("one line") :
                                     HunkWidget::tr("no line");
  QString deletionText = deletions ? deletions > 1 ? HunkWidget::tr("%1 lines").arg(deletions) :
                                                     HunkWidget::tr("one line") :
                                     HunkWidget::tr("no line");
  QString text = mPatch.isUntracked() ?
                 HunkWidget::tr("Are you sure you want to remove '%1'?").arg(name) :
                 HunkWidget::tr("Are you sure you want to discard the changes\n"
                                "from line %1 to line %2 "
                                "(%3 added, %4 removed)\n"
                                "in '%5'?")
                                  .arg(firstLine).arg(lastLine)
                                  .arg(additionText).arg(deletionText)
                                  .arg(name);

  QMessageBox *dialog = new QMessageBox(QMessageBox::Warning,
                                        title, text,
                                        QMessageBox::Cancel, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setInformativeText(HunkWidget::tr("This action cannot be undone."));

  QPushButton *discard = dialog->addButton(HunkWidget::tr("Discard selected lines"),
                                           QMessageBox::AcceptRole);
  connect(discard, &QPushButton::clicked, [this, startLine, end] {
    discardSelected(startLine, end);
  });

  dialog->open();
}

void HunkWidget::discardSelected(int startLine, int end)
{
  for (int i = startLine; i < end; i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Addition | 1 << TextEditor::Marker::Deletion))
      mEditor->markerAdd(i, TextEditor::DiscardMarker);
  }
  emit discard();
}

void HunkWidget::updatePatch(const git::Patch &patch,
                             const git::Patch &staged)
{
  // Update patch of unloaded hunk
  if (!mLoaded) {
    mPatch = patch;
    mStaged = staged;
  }
}

void HunkWidget::updateStageState(git::Index::StagedState stageState)
{
  if (!mLoaded)
    return;

  // Update stage state of loaded hunk
  if (stageState == git::Index::Staged ||
      stageState == git::Index::Unstaged)
  {
    mHeader->setStageState(stageState);

    // Update line markers
    bool staged = stageState == git::Index::Staged ? true : false;
    for (int i = 0; i < mEditor->lineCount(); i++) {
      int mask = mEditor->markers(i);
      if (mask & (1 << TextEditor::Marker::Addition | 1 << TextEditor::Marker::Deletion)) {
        if (staged) {
          if (!(mask & (1 << TextEditor::StagedMarker))) {
            mEditor->markerAdd(i, TextEditor::StagedMarker);
            if (mask & (1 << TextEditor::Marker::Addition))
              mStagedAdditions++;
            else
              mStagedDeletions++;
          }
        } else {
          if (mask & (1 << TextEditor::StagedMarker)) {
            mEditor->markerDelete(i, TextEditor::StagedMarker);
            if (mask & (1 << TextEditor::Marker::Addition))
              mStagedAdditions--;
            else
              mStagedDeletions--;
          }
        }
      }
    }
  }
}

void HunkWidget::marginClicked(int pos, int modifier, int margin)
{
  Q_UNUSED(modifier)

  if (margin != TextEditor::Margin::Staged)
    return;

  int lidx = mEditor->lineFromPosition(pos);
  int markers = mEditor->markers(lidx);
  if (!(markers & (1 << TextEditor::Marker::Addition | 1 << TextEditor::Marker::Deletion)))
    return;

  // Toggle staged line
  if (markers & (1 << TextEditor::Marker::StagedMarker))
    unstageSelected(lidx, lidx + 1);
  else
    stageSelected(lidx, lidx + 1);
}

int HunkWidget::tokenEndPosition(int pos) const
{
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

QList<HunkWidget::Token> HunkWidget::tokens(int line) const
{
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

QByteArray HunkWidget::tokenBuffer(const QList<HunkWidget::Token> &tokens)
{
  QByteArrayList list;
  foreach (const Token &token, tokens)
    list.append(token.text);
  return list.join('\n');
}

void HunkWidget::load(git::Patch &staged, bool force)
{
  if (!force && mLoaded )
    return;

  mLoaded = true;
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

  // Load hunk.
  QList<Line> lines;
  QByteArray content;

  // Create content for the editor
  int patchCount = mPatch.lineCount(mIndex);
  for (int lidx = 0; lidx < patchCount; ++lidx) {
    char origin = mPatch.lineOrigin(mIndex, lidx);
    bool EOL = false;
    if (origin == GIT_DIFF_LINE_CONTEXT_EOFNL ||
        origin == GIT_DIFF_LINE_ADD_EOFNL ||
        origin == GIT_DIFF_LINE_DEL_EOFNL) {
      Q_ASSERT(!lines.isEmpty());
      lines.last().setNewline(false);
      content += '\n';
      EOL = true;
    }

    if (!EOL) {
        int oldLine = mPatch.lineNumber(mIndex, lidx, git::Diff::OldFile);
        int newLine = mPatch.lineNumber(mIndex, lidx, git::Diff::NewFile);
        lines << Line(origin, oldLine, newLine);
        content += mPatch.lineContent(mIndex, lidx);
    }
  }

  // Trim final line end.
  if (content.endsWith('\n'))
     content.chop(1);
  if (content.endsWith('\r'))
     content.chop(1);

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

      default:
        break;
    }
  }

  // Execute hunk plugins.
  foreach (PluginRef plugin, mView->plugins()) {
    if (plugin->isValid() && plugin->isEnabled())
      plugin->hunk(mEditor);
  }

  mEditor->updateGeometry();

  // Update stage state after everything is loaded
  mHeader->setStageState(stageState());
}

void HunkWidget::setEditorLineInfos(QList<Line>& lines, Account::FileComments& comments, int width)
{
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

  // Staged line numbers
  const git_diff_hunk *unstageHeader = mPatch.header_struct(mIndex);

  QMap<int, QString> stagedAddLines;
  QMap<int, QString> stagedDelLines;
  if (!mPatch.isConflicted()) {
    for (int i = 0; i < mStaged.count(); i++) {

      // Find staged patch within patch
      const git_diff_hunk *stageHeader = mStaged.header_struct(i);
      int old_start = stageHeader->old_start - unstageHeader->old_start;
      int stageDiff = stageHeader->old_start - stageHeader->new_start;
      int unstageDiff = unstageHeader->old_start - unstageHeader->new_start;
      if ((old_start >= 0 && old_start <= unstageHeader->old_lines) &&
          (stageHeader->old_lines <= unstageHeader->old_lines)) {

        for (int lidx = 0; lidx < mStaged.lineCount(i); lidx++) {
          char origin = mStaged.lineOrigin(i, lidx);
          if (origin == '-')
            stagedDelLines.insert(mStaged.lineNumber(i, lidx, git::Diff::OldFile),
                                  mStaged.lineContent(i, lidx));

          if (origin == '+')
            stagedAddLines.insert(mStaged.lineNumber(i, lidx, git::Diff::NewFile)
                                  + stageDiff - unstageDiff,
                                  mStaged.lineContent(i, lidx));
        }
      }
    }
  }

  // Reset statistics
  mAdditions = 0, mDeletions = 0;
  mStagedAdditions = 0, mStagedDeletions = 0;

  // Set all editor markers, like green background for adding, red background for deletion
  // Blue background for OURS or magenta background for Theirs
  // Setting also the staged symbol
  int count = lines.size();
  for (int lidx = 0; lidx < count; ++lidx) {
    int additions = 0, deletions = 0;
    int marker = -1;
    bool staged = false;

    const Line& line = lines.at(lidx);
    createMarkersAndLineNumbers(line, lidx, comments, width);

    switch (lines[lidx].origin()) {
      case GIT_DIFF_LINE_CONTEXT:
        marker = TextEditor::Context;
        additions = 0;
        deletions = 0;
        break;

      case GIT_DIFF_LINE_ADDITION: {
        marker = TextEditor::Addition;
        additions++;
        // Find matching lines
        if (lidx + 1 >= count ||
            mPatch.lineOrigin(mIndex, lidx + 1) != GIT_DIFF_LINE_ADDITION) { // end of file, or the last addition line
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
        int patchLinenumber = mPatch.lineNumber(mIndex, lidx, git::Diff::NewFile) - (mAdditions - mStagedAdditions) + (mDeletions - mStagedDeletions);
        if (stagedAddLines.contains(patchLinenumber)) {
          if (stagedAddLines.value(patchLinenumber) == mPatch.lineContent(mIndex, lidx)) {
            mStagedAdditions++;
            staged = true;
          }
        }

        mAdditions++;
        break;
      }

      case GIT_DIFF_LINE_DELETION: {
        marker = TextEditor::Deletion;
        deletions++;
        mDeletions++;

        // Check if staged
        int patchLinenumber = mPatch.lineNumber(mIndex, lidx, git::Diff::OldFile);
        if (stagedDelLines.contains(patchLinenumber)) {
          if (stagedDelLines.value(patchLinenumber) == mPatch.lineContent(mIndex, lidx)) {
            mStagedDeletions++;
            staged = true;
          }
        }
        break;
      }

      case 'O':
        marker = TextEditor::Ours;
        break;

      case 'T':
        marker = TextEditor::Theirs;
        break;
    }

    // Add marker.
    if (marker >= 0)
      mEditor->markerAdd(lidx, marker);
    if (staged)
      mEditor->markerAdd(lidx, TextEditor::StagedMarker);
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

void HunkWidget::createMarkersAndLineNumbers(const Line& line, int lidx, Account::FileComments& comments, int width) const
{
    QByteArray oldLine = line.oldLine();
    QByteArray newLine = line.newLine();
    int spaces = width - (oldLine.length() + newLine.length());
    QByteArray text = oldLine + QByteArray(spaces, ' ') + newLine;
    mEditor->marginSetText(lidx, text);
    mEditor->marginSetStyle(lidx, STYLE_LINENUMBER);

    // Build annotations.
    QList<Annotation> annotations;
    if (!line.newline()) {
      QString text = tr("No newline at end of file");
      QByteArray styles =
        QByteArray(text.toUtf8().size(), TextEditor::EofNewline);
      annotations.append({text, styles});
    }

    auto it = comments.constFind(lidx);
    if (it != comments.constEnd()) {
      QString whitespace(DiffViewStyle::kIndent, ' ');
      QFont font = mEditor->styleFont(TextEditor::CommentBody);
      int margin = QFontMetrics(font).horizontalAdvance(' ') * DiffViewStyle::kIndent * 2;
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
        QString time = key.toString(Qt::DefaultLocaleLongDate);
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

QList<int> HunkWidget::unstagedLines() const
{
  QList<int> list;
  for (int i = 0; i < mEditor->lineCount(); i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Context))
      continue;
    if (mask & (1 << TextEditor::Marker::Addition | 1 << TextEditor::Marker::Deletion))
      if (!(mask & (1 << TextEditor::Marker::StagedMarker)))
        list.append(i);
  }

  return list;
}

QList<int> HunkWidget::discardedLines() const
{
  QList<int> list;
  for (int i = 0; i < mEditor->lineCount(); i++) {
    int mask = mEditor->markers(i);
    if (mask & (1 << TextEditor::Marker::Context))
      continue;
    if (mask & (1 << TextEditor::Marker::DiscardMarker))
      list.append(i);
  }

  return list;
}

git::Index::StagedState HunkWidget::stageState()
{
  if ((mStagedAdditions + mStagedDeletions) == 0)
    return git::Index::Unstaged;
  else if ((mStagedAdditions + mStagedDeletions) == (mAdditions + mDeletions))
    return git::Index::Staged;

  return git::Index::PartiallyStaged;
}

void HunkWidget::chooseLines(TextEditor::Marker kind)
{
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
