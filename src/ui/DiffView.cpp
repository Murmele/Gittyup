//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DiffView.h"
#include "Badge.h"
#include "BlameEditor.h"
#include "EditorWindow.h"
#include "FileContextMenu.h"
#include "MenuBar.h"
#include "RepoView.h"
#include "app/Application.h"
#include "conf/Settings.h"
#include "git/Blame.h"
#include "git/Blob.h"
#include "git/Branch.h"
#include "git/Buffer.h"
#include "git/Commit.h"
#include "git/FilterList.h"
#include "git/Index.h"
#include "git/Patch.h"
#include "git/Repository.h"
#include "git/Signature.h"
#include "git/Submodule.h"
#include "git/Tree.h"
#include "git2/diff.h"
#include "git2/index.h"
#include "log/LogEntry.h"
#include <QCheckBox>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QShortcut>
#include <QStyleOption>
#include <QTableWidget>
#include <QTextEdit>
#include <QTextLayout>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {

const int kIndent = 2;
const int kArrowWidth = 20;
const int kArrowMargin = 6;
const QString kHunkFmt = "<h4>%1</h4>";

const QString kStyleSheet =
  "DiffView {"
  "  border-image: url(:/sunken.png) 0 0 0 0;"
  "  border-left-width: 4;"
  "  border-top-width: 0;"
  "  padding-left: -4;"
  "  padding-top: 0"
  "}"
  "DiffView HunkWidget, DiffView .QFrame {"
  "  background-clip: content;"
  "  border-image: url(:/shadow.png) 0 0 0 0;"
  "  border-width: 0;"
  "  padding-left: 10;"
  "  padding-top: 0"
  "}";

const QString kButtonStyleFmt =
  "QToolButton {"
  "  background: %1"
  "}"
  "QToolButton:pressed {"
  "  background: %2"
  "}";

const QDir::Filters kFilters =
  QDir::Files |
  QDir::Dirs |
  QDir::Hidden |
  QDir::NoDotAndDotDot;

const QUrl::FormattingOptions kUrlFormat =
  QUrl::PreferLocalFile |
  QUrl::StripTrailingSlash |
  QUrl::NormalizePathSegments;

bool copy(const QString &source, const QDir &targetDir)
{
  // Disallow copy into self.
  if (source.startsWith(targetDir.path()))
    return false;

  QFileInfo info(source);
  QString name = info.fileName();
  QString target = targetDir.filePath(name);
  if (!info.isDir())
    return QFile::copy(source, target);

  if (!targetDir.mkdir(name))
    return false;

  foreach (const QFileInfo &entry, QDir(source).entryInfoList(kFilters)) {
    if (!copy(entry.filePath(), target))
      return false;
  }

  return true;
}

QString buttonStyle(Theme::Diff role)
{
  Theme *theme = Application::theme();
  QColor color = theme->diff(role);
  QString pressed = color.darker(115).name();
  return kButtonStyleFmt.arg(color.name(), pressed);
}

struct Annotation
{
  QString text;
  QByteArray styles;
};

class Comment : public QTextEdit
{
public:
  Comment(
    const QDateTime &date,
    const Account::Comment &comment,
    QWidget *parent = nullptr)
    : QTextEdit(parent)
  {
    setReadOnly(true);
    setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QTextCursor cursor = textCursor();
    Theme *theme = Application::theme();

    QTextCharFormat author;
    author.setForeground(theme->remoteComment(Theme::Comment::Author));
    author.setFontWeight(QFont::Bold);
    cursor.setCharFormat(author);
    cursor.insertText(comment.author + " ");

    QTextCharFormat timestamp;
    timestamp.setForeground(theme->remoteComment(Theme::Comment::Timestamp));
    cursor.setCharFormat(timestamp);
    cursor.insertText(date.toString(Qt::DefaultLocaleLongDate));

    QTextBlockFormat indent;
    indent.setLeftMargin(fontMetrics().horizontalAdvance(' ') * kIndent);
    cursor.insertBlock(indent);

    QTextCharFormat body;
    body.setForeground(theme->remoteComment(Theme::Comment::Body));
    cursor.setCharFormat(body);
    cursor.insertText(comment.body);

    connect(verticalScrollBar(), &QScrollBar::rangeChanged, [this] {
      updateGeometry();
    });
  }

  QSize minimumSizeHint() const override
  {
    return QSize();
  }

  QSize viewportSizeHint() const override
  {
    return document()->size().toSize();
  }
};

class CommentWidget : public QWidget
{
public:
  CommentWidget(const Account::Comments &comments, QWidget *parent = nullptr)
    : QWidget(parent)
  {
    setStyleSheet("QWidget { background-color: transparent; border: none }");
    setContentsMargins(4,4,4,4);

    QVBoxLayout *layout = new QVBoxLayout(this);
    foreach (const QDateTime &key, comments.keys())
      layout->addWidget(new Comment(key, comments.value(key), this));
  }
};

class Line
{
public:
  Line(char origin, int oldLine, int newLine)
    : mOrigin(origin)
  {
    mOldLine = (oldLine >= 0) ? QByteArray::number(oldLine) : QByteArray();
    mNewLine = (newLine >= 0) ? QByteArray::number(newLine) : QByteArray();
  }

  char origin() const { return mOrigin; }
  QByteArray oldLine() const { return mOldLine; }
  QByteArray newLine() const { return mNewLine; }

  bool newline() const { return mNewline; }
  void setNewline(bool newline) { mNewline = newline; }

  int matchingLine() const { return mMatchingLine; }
  void setMatchingLine(int line) { mMatchingLine = line; }

private:
  char mOrigin = -1;
  bool mNewline = true;
  int mMatchingLine = -1;
  QByteArray mOldLine;
  QByteArray mNewLine;
};

class Button : public QToolButton
{
public:
  Button(QWidget *parent = nullptr)
    : QToolButton(parent)
  {}

  QSize sizeHint() const override
  {
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    // Start with the check box dimensions.
    int width = style()->pixelMetric(QStyle::PM_IndicatorWidth, &opt, this);
    int height = style()->pixelMetric(QStyle::PM_IndicatorHeight, &opt, this);
    return QSize(width + 2, height + 4);
  }

protected:
  void initButtonPainter(QPainter *painter)
  {
    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen = painter->pen();
    pen.setWidth(2);
    painter->setPen(pen);
  }
};

class EditButton : public Button
{
public:
  EditButton(
    const git::Patch &patch,
    int index,
    bool binary,
    bool lfs,
    QWidget *parent = nullptr)
    : Button(parent)
  {
    setObjectName("EditButton");

    // Add edit button menu.
    QMenu *menu = new QMenu(this);

    QString name = patch.name();
    RepoView *view = RepoView::parentView(this);

    // Calculate starting line numbers.
    int oldLine = -1;
    int newLine = -1;
    if (index >= 0 && patch.lineCount(index) > 0) {
      oldLine = patch.lineNumber(index, 0, git::Diff::OldFile);
      newLine = patch.lineNumber(index, 0, git::Diff::NewFile);
    }

    if (view->repo().workdir().exists(name)) {
      QAction *action = menu->addAction("Edit Working Copy");
      connect(action, &QAction::triggered, [this, view, name, newLine] {
        view->edit(name, newLine);
      });
    }

    // Add revision edit actions.
    QList<git::Commit> commits = view->commits();
    git::Commit commit = !commits.isEmpty() ? commits.first() : git::Commit();
    git::Blob newBlob = patch.blob(git::Diff::NewFile);
    if (newBlob.isValid()) {
      QAction *action = menu->addAction("Edit New Revision");
      connect(action, &QAction::triggered,
      [this, view, name, newLine, newBlob, commit] {
        view->openEditor(name, newLine, newBlob, commit);
      });
    }

    git::Blob oldBlob = patch.blob(git::Diff::OldFile);
    if (oldBlob.isValid()) {
      if (commit.isValid()) {
        QList<git::Commit> parents = commit.parents();
        if (!parents.isEmpty())
          commit = parents.first();
      }

      QAction *action = menu->addAction("Edit Old Revision");
      connect(action, &QAction::triggered,
      [this, view, name, oldLine, oldBlob, commit] {
        view->openEditor(name, oldLine, oldBlob, commit);
      });
    }

    // Connect button click to the first menu action.
    setEnabled(!menu->isEmpty() && !binary && !lfs);
    connect(this, &EditButton::clicked, [menu] {
      menu->actions().first()->trigger();
    });

    setMenu(menu);
  }

protected:
  void paintEvent(QPaintEvent *event) override
  {
    QPainter painter(this);
    initButtonPainter(&painter);

    qreal r = 6;
    qreal x = width() / 2;
    qreal y = height() / 2;

    QPainterPath path;
    path.moveTo(x - r, y + r);
    path.lineTo(x - r, y + r - 4);
    path.lineTo(x + r - 4, y - r);
    path.lineTo(x + r, y - r + 4);
    path.lineTo(x - r + 4, y + r);
    path.lineTo(x - r, y + r);
    painter.drawPath(path);

    painter.setPen(QPen(painter.pen().color(), 1, Qt::SolidLine, Qt::FlatCap));
    painter.drawLine(x - r + 1, y + r - 5, x - r + 5, y + r - 1);
    painter.drawLine(x + r - 6, y - r + 2, x + r - 2, y - r + 6);
  }
};

class DiscardButton : public Button
{
public:
  DiscardButton(QWidget *parent = nullptr)
    : Button(parent)
  {
    setObjectName("DiscardButton");
  }

protected:
  void paintEvent(QPaintEvent *event) override
  {
    QPainter painter(this);
    initButtonPainter(&painter);

    qreal r = 4;
    qreal x = width() / 2;
    qreal y = height() / 2;

    QPainterPath path;
    path.moveTo(x - r, y - r);
    path.lineTo(x + r, y + r);
    path.moveTo(x + r, y - r);
    path.lineTo(x - r, y + r);
    painter.drawPath(path);
  }
};

class DisclosureButton : public Button
{
public:
  DisclosureButton(QWidget *parent = nullptr)
    : Button(parent)
  {
    setCheckable(true);
    setChecked(true);
  }

protected:
  void paintEvent(QPaintEvent *event) override
  {
    QPainter painter(this);
    initButtonPainter(&painter);

    qreal x = width() / 2;
    qreal y = height() / 2;

    QPainterPath path;
    if (isChecked()) {
      path.moveTo(x - 3.5, y - 2);
      path.lineTo(x, y + 2);
      path.lineTo(x + 3.5, y - 2);
    } else {
      path.moveTo(x - 2, y + 3.5);
      path.lineTo(x + 2, y);
      path.lineTo(x - 2, y - 3.5);
    }

    painter.drawPath(path);
  }
};

class Editor : public TextEditor
{
public:
  Editor(QWidget *parent = nullptr)
    : TextEditor(parent)
  {}

protected:
  void focusOutEvent(QFocusEvent *event) override
  {
    if (event->reason() != Qt::PopupFocusReason)
      clearSelections();

    TextEditor::focusOutEvent(event);
  }
};

class Images : public QWidget
{
  Q_OBJECT

public:
  Images(
    const git::Patch patch,
    bool lfs = false,
    QWidget *parent = nullptr)
    : QWidget(parent), mPatch(patch)
  {
    int size = 0;
    QPixmap pixmap = loadPixmap(git::Diff::NewFile, size, lfs);
    if (pixmap.isNull()) {
      QString path = mPatch.repo().workdir().filePath(mPatch.name());
      size = QFileInfo(path).size();
      pixmap.load(path);
    }

    if (pixmap.isNull())
      return;

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 8, 4);

    int beforeSize = 0;
    QPixmap before = loadPixmap(git::Diff::OldFile, beforeSize, lfs);
    if (!before.isNull()) {
      layout->addLayout(imageLayout(before, beforeSize), 1);
      layout->addWidget(new Arrow(this));
    }

    layout->addLayout(imageLayout(pixmap, size), 1);
    layout->addStretch();
  }

private:
  class Image : public QWidget
  {
  public:
    Image(const QPixmap &pixmap, QWidget *parent = nullptr)
      : QWidget(parent), mPixmap(pixmap)
    {
      setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    }

    QSize sizeHint() const override
    {
      return mPixmap.size();
    }

    bool hasHeightForWidth() const override
    {
      return true;
    }

    int heightForWidth(int w) const override
    {
      QSize size = sizeHint();
      int width = size.width();
      int height = size.height();
      return (w >= width) ? height : height * (w / (qreal) width);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
      QPainter painter(this);
      painter.drawPixmap(rect(), mPixmap);
    }

  private:
    QPixmap mPixmap;
  };

  class Arrow : public QWidget
  {
  public:
    Arrow(QWidget *parent = nullptr)
      : QWidget(parent)
    {
      setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    }

    QSize sizeHint() const override
    {
      return QSize(100, 100);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
      QPainter painter(this);
      painter.setRenderHint(QPainter::Antialiasing);

      int x = width() / 2;
      int y = height() / 2;

      QSize size = sizeHint();
      int width = size.width();
      int height = size.height();

      QPainterPath arrow;
      arrow.moveTo(x - width * 0.40, y - height * 0.15);
      arrow.lineTo(x + width * 0.05, y - height * 0.15);
      arrow.lineTo(x + width * 0.05, y - height * 0.27);
      arrow.lineTo(x + width * 0.40, y);
      arrow.lineTo(x + width * 0.05, y + height * 0.27);
      arrow.lineTo(x + width * 0.05, y + height * 0.15);
      arrow.lineTo(x - width * 0.40, y + height * 0.15);
      arrow.closeSubpath();

      QPalette palette = Application::palette();
      painter.setPen(QPen(palette.brightText(), 2.0));
      painter.setBrush(palette.button());
      painter.drawPath(arrow);
    }
  };

  QPixmap loadPixmap(git::Diff::File type, int &size, bool lfs)
  {
    git::Blob blob = mPatch.blob(type);
    if (!blob.isValid())
      return QPixmap();

    QByteArray data = blob.content();

    if (lfs)
      data = mPatch.repo().lfsSmudge(data, mPatch.name());

    size = data.length();

    QPixmap pixmap;
    pixmap.loadFromData(data);
    if (!pixmap.isNull())
      return pixmap;

    QFileIconProvider provider;
    QString path = mPatch.repo().workdir().filePath(mPatch.name());
    QIcon icon = provider.icon(QFileInfo(path));
    return icon.pixmap(windowHandle(), QSize(64, 64));
  }

  QVBoxLayout *imageLayout(const QPixmap pixmap, int size)
  {
    QString arg = locale().formattedDataSize(size);
    QLabel *label = new QLabel(tr("<b>Size:</b> %1").arg(arg));
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(new Image(pixmap, this));
    layout->addStretch();

    return layout;
  }

  git::Patch mPatch;
};

class HunkWidget : public QFrame
{
  Q_OBJECT

public:
  class Header : public QWidget
  {
  public:
    Header(
      const git::Diff &diff,
      const git::Patch &patch,
      int index,
      bool lfs,
      bool submodule,
      QWidget *parent = nullptr)
      : QWidget(parent)
    {
      setObjectName("HunkHeader");
      mCheck = new QCheckBox(this);

      QString header = (index >= 0) ? patch.header(index) : QString();
      QString escaped = header.trimmed().toHtmlEscaped();
      QLabel *label = new QLabel(kHunkFmt.arg(escaped), this);

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
        connect(discard, &DiscardButton::clicked, [this, patch, index] {
          QString name = patch.name();
          int line = patch.lineNumber(index, 0, git::Diff::NewFile);

          QString title = HunkWidget::tr("Discard Hunk?");
          QString text = patch.isUntracked() ?
            HunkWidget::tr("Are you sure you want to remove '%1'?").arg(name) :
            HunkWidget::tr("Are you sure you want to discard the "
               "hunk starting at line %1 in '%2'?").arg(line).arg(name);

          QMessageBox *dialog = new QMessageBox(
            QMessageBox::Warning, title, text, QMessageBox::Cancel, this);
          dialog->setAttribute(Qt::WA_DeleteOnClose);
          dialog->setInformativeText(HunkWidget::tr("This action cannot be undone."));

          QPushButton *discard =
            dialog->addButton(HunkWidget::tr("Discard Hunk"), QMessageBox::AcceptRole);
          connect(discard, &QPushButton::clicked, [this, patch, index] {
            git::Repository repo = patch.repo();
            if (patch.isUntracked()) {
              repo.workdir().remove(patch.name());
              return;
            }

            QString name = patch.name();
            QSaveFile file(repo.workdir().filePath(name));
            if (!file.open(QFile::WriteOnly))
              return;

            QBitArray hunks(patch.count(), true);
            hunks[index] = false;
            QByteArray buffer = patch.apply(hunks, repo.filters(name));
            if (buffer.isEmpty())
              return;

            file.write(buffer);
            if (!file.commit())
              return;

            // FIXME: Work dir changed?
            RepoView::parentView(this)->refresh();
          });

          dialog->open();
        });
      }

      mButton = new DisclosureButton(this);
      mButton->setToolTip(
        mButton->isChecked() ? HunkWidget::tr("Collapse Hunk") : HunkWidget::tr("Expand Hunk"));
      connect(mButton, &DisclosureButton::toggled, [this] {
        mButton->setToolTip(
          mButton->isChecked() ? HunkWidget::tr("Collapse Hunk") : HunkWidget::tr("Expand Hunk"));
      });

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

      // Collapse on check.
      connect(mCheck, &QCheckBox::clicked, [this](bool staged) {
        mButton->setChecked(!staged);
      });
    }

    QCheckBox *check() const { return mCheck; }

    DisclosureButton *button() const { return mButton; }

    QToolButton *saveButton() const { return mSave; }
    QToolButton *undoButton() const { return mUndo; }
    QToolButton *oursButton() const { return mOurs; }
    QToolButton *theirsButton() const { return mTheirs; }

  protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
      if (mButton->isEnabled())
        mButton->toggle();
    }

  private:
    QCheckBox *mCheck;
    DisclosureButton *mButton;
    QToolButton *mSave = nullptr;
    QToolButton *mUndo = nullptr;
    QToolButton *mOurs = nullptr;
    QToolButton *mTheirs = nullptr;
  };

  HunkWidget(
    DiffView *view,
    const git::Diff &diff,
    const git::Patch &patch,
    int index,
    bool lfs,
    bool submodule,
    QWidget *parent = nullptr)
    : QFrame(parent), mView(view), mPatch(patch), mIndex(index)
  {
    setObjectName("HunkWidget");
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(4);

    mHeader = new Header(diff, patch, index, lfs, submodule, this);
    layout->addWidget(mHeader);

    mEditor = new Editor(this);
    mEditor->setLexer(patch.name());
    mEditor->setCaretStyle(CARETSTYLE_INVISIBLE);
    mEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    if (index >= 0)
      mEditor->setLineCount(patch.lineCount(index));

    connect(mEditor, &TextEditor::updateUi,
            MenuBar::instance(this), &MenuBar::updateCutCopyPaste);

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
    connect(mHeader->button(), &DisclosureButton::toggled,
            mEditor, &TextEditor::setVisible);

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
    connect(mEditor, &TextEditor::marginClicked, [this, status](int pos) {
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

  Header *header() const { return mHeader; }

  TextEditor *editor(bool ensureLoaded = true) {
    if (ensureLoaded)
      load();
    return mEditor;
  }

  void invalidate()
  {
    mEditor->setReadOnly(false);
    mEditor->clearAll();
    mLoaded = false;
    update();
  }

protected:
  void paintEvent(QPaintEvent *event) override
  {
    load();
    QFrame::paintEvent(event);
  }

private:
  struct Token
  {
    int pos;
    QByteArray text;
  };

  int tokenEndPosition(int pos) const
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

  QList<Token> tokens(int line) const
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

  QByteArray tokenBuffer(const QList<Token> &tokens)
  {
    QByteArrayList list;
    foreach (const Token &token, tokens)
      list.append(token.text);
    return list.join('\n');
  }

  void load()
  {
    if (mLoaded)
      return;

    mLoaded = true;

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

    // Load hunk.
    QList<Line> lines;
    QByteArray content;
    int patchCount = mPatch.lineCount(mIndex);
    for (int lidx = 0; lidx < patchCount; ++lidx) {
      char origin = mPatch.lineOrigin(mIndex, lidx);
      if (origin == GIT_DIFF_LINE_CONTEXT_EOFNL ||
          origin == GIT_DIFF_LINE_ADD_EOFNL ||
          origin == GIT_DIFF_LINE_DEL_EOFNL) {
        Q_ASSERT(!lines.isEmpty());
        lines.last().setNewline(false);
        content += '\n';
        continue;
      }

      int oldLine = mPatch.lineNumber(mIndex, lidx, git::Diff::OldFile);
      int newLine = mPatch.lineNumber(mIndex, lidx, git::Diff::NewFile);
      lines << Line(origin, oldLine, newLine);
      content += mPatch.lineContent(mIndex, lidx);
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

    // Add markers and line numbers.
    int additions = 0;
    int deletions = 0;
    int count = lines.size();
    for (int lidx = 0; lidx < count; ++lidx) {
      const Line &line = lines.at(lidx);
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
        QString whitespace(kIndent, ' ');
        QFont font = mEditor->styleFont(TextEditor::CommentBody);
        int margin = QFontMetrics(font).horizontalAdvance(' ') * kIndent * 2;
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

      // Find matching lines.
      int marker = -1;
      switch (line.origin()) {
        case GIT_DIFF_LINE_CONTEXT:
          marker = TextEditor::Context;
          additions = 0;
          deletions = 0;
          break;

        case GIT_DIFF_LINE_ADDITION:
          marker = TextEditor::Addition;
          ++additions;
          if (lidx + 1 >= count ||
              mPatch.lineOrigin(mIndex, lidx + 1) != GIT_DIFF_LINE_ADDITION) {
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

            additions = 0;
            deletions = 0;
          }
          break;

        case GIT_DIFF_LINE_DELETION:
          marker = TextEditor::Deletion;
          ++deletions;
          break;

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
    }

    // Diff matching lines.
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
  }

  void chooseLines(TextEditor::Marker kind)
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

  DiffView *mView;
  git::Patch mPatch;
  int mIndex;

  Header *mHeader;
  TextEditor *mEditor;
  bool mLoaded = false;
};

class LineStats : public QWidget
{
public:
  LineStats(const git::Patch::LineStats &stats, QWidget *parent = nullptr)
    : QWidget(parent)
  {
    Q_ASSERT(stats.additions > 0 || stats.deletions > 0);

    qreal x = static_cast<qreal>(stats.additions + stats.deletions) / 5;
    bool scaled = (stats.additions + stats.deletions > 5);
    mPluses = scaled ? stats.additions / x : stats.additions;
    mMinuses = scaled ? stats.deletions / x : stats.deletions;
  }

  QSize minimumSizeHint() const override
  {
    return QSize(10 * (mPluses + mMinuses), 12);
  }

  void paintEvent(QPaintEvent *event) override
  {
    // Drawing the outline of the shapes with a narrow pen approximates
    // a slight drop shadow. Minus has to be drawn slightly smaller than
    // plus or it creates an optical illusion that makes it look to big.

    QPainter painter(this);
    qreal w = width() / (mPluses + mMinuses);
    qreal h = height();
    qreal x = w / 2;
    qreal y = h / 2;

    painter.setPen(QPen(Qt::black, 0.1));
    painter.setBrush(Application::theme()->diff(Theme::Diff::Plus));
    for (int i = 0; i < mPluses; ++i) {
      QPainterPath path;
      path.moveTo(x - 4, y - 1);
      path.lineTo(x - 1, y - 1);
      path.lineTo(x - 1, y - 4);
      path.lineTo(x + 1, y - 4);
      path.lineTo(x + 1, y - 1);
      path.lineTo(x + 4, y - 1);
      path.lineTo(x + 4, y + 1);
      path.lineTo(x + 1, y + 1);
      path.lineTo(x + 1, y + 4);
      path.lineTo(x - 1, y + 4);
      path.lineTo(x - 1, y + 1);
      path.lineTo(x - 4, y + 1);
      path.closeSubpath();
      painter.drawPath(path);
      x += w;
    }

    painter.setBrush(Application::theme()->diff(Theme::Diff::Minus));
    for (int i = 0; i < mMinuses; ++i) {
      QPainterPath path;
      path.moveTo(x - 3, y - 1);
      path.lineTo(x + 4, y - 1);
      path.lineTo(x + 4, y + 1);
      path.lineTo(x - 3, y + 1);
      path.closeSubpath();
      painter.drawPath(path);
      x += w;
    }
  }

private:
  int mPluses = 0;
  int mMinuses = 0;
};

class FileLabel : public QWidget
{
public:
  FileLabel(const QString &name, bool submodule, QWidget *parent = nullptr)
    : QWidget(parent), mName(name)
  {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QFont font = this->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 3);
    if (submodule) {
      font.setUnderline(true);
      setCursor(Qt::PointingHandCursor);
    }

    setFont(font);
  }

  void setOldName(const QString &oldName)
  {
    mOldName = oldName;
  }

  QSize sizeHint() const override
  {
    QFontMetrics fm = fontMetrics();
    int width = fm.boundingRect(mName).width() + 2;
    if (!mOldName.isEmpty())
      width += fm.boundingRect(mOldName).width() + kArrowWidth;
    return QSize(width, fm.lineSpacing());
  }

protected:
  void paintEvent(QPaintEvent *event) override
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect rect = this->rect();
    QFontMetrics fm = fontMetrics();

    if (!mOldName.isEmpty()) {
      // Draw old name.
      int max = (width() - kArrowWidth) / 2;
      QString oldName = fm.elidedText(mOldName, Qt::ElideLeft, max);
      painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, oldName);
      rect.adjust(fm.boundingRect(oldName).width(), 0, 0, 0);

      // Draw arrow.
      int x1 = rect.x() + kArrowMargin;
      int x2 = rect.x() + kArrowWidth - kArrowMargin;
      int y = rect.height() / 2;
      QPainterPath path;
      path.moveTo(x1, y);
      path.lineTo(x2, y);
      path.moveTo(x2 - 3, y - 3);
      path.lineTo(x2, y);
      path.lineTo(x2 - 3, y + 3);
      QPen pen = painter.pen();
      pen.setWidthF(1.5);
      painter.setPen(pen);
      painter.drawPath(path);
      rect.adjust(kArrowWidth, 0, 0, 0);
    }

    QString name = fm.elidedText(mName, Qt::ElideLeft, rect.width());
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, name);
  }

  void mouseReleaseEvent(QMouseEvent *event) override
  {
    if (!rect().contains(event->pos()))
      return;

    QUrl url;
    url.setScheme("submodule");
    url.setPath(mName);
    RepoView::parentView(this)->visitLink(url.toString());
  }

private:
  QString mName;
  QString mOldName;
};

class FileWidget : public QWidget
{
  Q_OBJECT

public:
  class Header : public QFrame
  {
  public:
    Header(
      const git::Diff &diff,
      const git::Patch &patch,
      bool binary,
      bool lfs,
      bool submodule,
      QWidget *parent = nullptr)
      : QFrame(parent), mDiff(diff), mPatch(patch)
    {
      setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

      QString name = patch.name();
      mCheck = new QCheckBox(this);
      mCheck->setVisible(diff.isStatusDiff());

      char status = git::Diff::statusChar(patch.status());
      Badge *badge = new Badge({Badge::Label(QChar(status))}, this);

      LineStats *stats = nullptr;
      git::Patch::LineStats lineStats = patch.lineStats();
      if (lineStats.additions > 0 || lineStats.deletions > 0)
        stats = new LineStats(lineStats, this);

      FileLabel *label = new FileLabel(name, submodule, this);
      if (patch.status() == GIT_DELTA_RENAMED)
        label->setOldName(patch.name(git::Diff::OldFile));

      QHBoxLayout *buttons = new QHBoxLayout;
      buttons->setContentsMargins(0,0,0,0);
      buttons->setSpacing(4);

      QHBoxLayout *layout = new QHBoxLayout(this);
      layout->setContentsMargins(4,4,4,4);
      layout->addWidget(mCheck);
      layout->addSpacing(4);
      layout->addWidget(badge);
      if (stats)
        layout->addWidget(stats);
      layout->addWidget(label, 1);
      layout->addStretch();
      layout->addLayout(buttons);

      // Add LFS buttons.
      if (lfs) {
        Badge *lfsBadge = new Badge({Badge::Label(FileWidget::tr("LFS"), true)}, this);
        buttons->addWidget(lfsBadge);

        QToolButton *lfsLockButton = new QToolButton(this);
        bool locked = patch.repo().lfsIsLocked(patch.name());
        lfsLockButton->setText(locked ? FileWidget::tr("Unlock") : FileWidget::tr("Lock"));
        buttons->addWidget(lfsLockButton);

        connect(lfsLockButton, &QToolButton::clicked, [this, patch] {
          bool locked = patch.repo().lfsIsLocked(patch.name());
          RepoView::parentView(this)->lfsSetLocked({patch.name()}, !locked);
        });

        git::RepositoryNotifier *notifier = patch.repo().notifier();
        connect(notifier, &git::RepositoryNotifier::lfsLocksChanged, this,
        [this, patch, lfsLockButton] {
          bool locked = patch.repo().lfsIsLocked(patch.name());
          lfsLockButton->setText(locked ? FileWidget::tr("Unlock") : FileWidget::tr("Lock"));
        });

        mLfsButton = new QToolButton(this);
        mLfsButton->setText(FileWidget::tr("Show Object"));
        mLfsButton->setCheckable(true);

        buttons->addWidget(mLfsButton);
        buttons->addSpacing(8);
      }

      // Add edit button.
      mEdit = new EditButton(patch, -1, binary, lfs, this);
      mEdit->setToolTip(FileWidget::tr("Edit File"));
      buttons->addWidget(mEdit);

      // Add discard button.
      if (diff.isStatusDiff() && !submodule && !patch.isConflicted()) {
        DiscardButton *discard = new DiscardButton(this);
        discard->setToolTip(FileWidget::tr("Discard File"));
        buttons->addWidget(discard);

        connect(discard, &QToolButton::clicked, [this] {
          QString name = mPatch.name();
          bool untracked = mPatch.isUntracked();
          QString path = mPatch.repo().workdir().filePath(name);
          QString arg = QFileInfo(path).isDir() ? FileWidget::tr("Directory") : FileWidget::tr("File");
          QString title =
            untracked ? FileWidget::tr("Remove %1?").arg(arg) : FileWidget::tr("Discard Changes?");
          QString text = untracked ?
            FileWidget::tr("Are you sure you want to remove '%1'?") :
            FileWidget::tr("Are you sure you want to discard all changes in '%1'?");
          QMessageBox *dialog = new QMessageBox(
            QMessageBox::Warning, title, text.arg(name),
            QMessageBox::Cancel, this);
          dialog->setAttribute(Qt::WA_DeleteOnClose);
          dialog->setInformativeText(FileWidget::tr("This action cannot be undone."));

          QString button =
            untracked ? FileWidget::tr("Remove %1").arg(arg) : FileWidget::tr("Discard Changes");
          QPushButton *discard =
            dialog->addButton(button, QMessageBox::AcceptRole);
          connect(discard, &QPushButton::clicked, [this, untracked] {
            RepoView *view = RepoView::parentView(this);
            git::Repository repo = mPatch.repo();
            QString name = mPatch.name();
            int strategy = GIT_CHECKOUT_FORCE;
            if (untracked) {
              QDir dir = repo.workdir();
              if (QFileInfo(dir.filePath(name)).isDir()) {
                if (dir.cd(name))
                  dir.removeRecursively();
              } else {
                dir.remove(name);
              }
            } else if (!repo.checkout(git::Commit(), nullptr, {name}, strategy)) {
              LogEntry *parent = view->addLogEntry(mPatch.name(), FileWidget::tr("Discard"));
              view->error(parent, FileWidget::tr("discard"), mPatch.name());
            }

            // FIXME: Work dir changed?
            view->refresh();
          });

          dialog->open();
        });
      }

      mDisclosureButton = new DisclosureButton(this);
      mDisclosureButton->setToolTip(
        mDisclosureButton->isChecked() ? FileWidget::tr("Collapse File") : FileWidget::tr("Expand File"));
      connect(mDisclosureButton, &DisclosureButton::toggled, [this] {
        mDisclosureButton->setToolTip(
          mDisclosureButton->isChecked() ? FileWidget::tr("Collapse File") : FileWidget::tr("Expand File"));
      });
      buttons->addWidget(mDisclosureButton);

      if (!diff.isStatusDiff())
        return;

      // Respond to check changes.
      connect(mCheck, &QCheckBox::clicked, [this](bool staged) {
        mCheck->setChecked(!staged); // Allow index to decide.
        mDiff.index().setStaged({mPatch.name()}, staged);
      });

      // Respond to index changes.
      git::Repository repo = RepoView::parentView(this)->repo();
      connect(repo.notifier(), &git::RepositoryNotifier::indexChanged, this,
      [this](const QStringList &paths) {
        if (paths.contains(mPatch.name()))
          updateCheckState();
      });

      // Set initial check state.
      updateCheckState();
    }

    QCheckBox *check() const { return mCheck; }

    DisclosureButton *disclosureButton() const { return mDisclosureButton; }

    QToolButton *lfsButton() const { return mLfsButton; }

  protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
      if (mDisclosureButton->isEnabled())
        mDisclosureButton->toggle();
    }

    void contextMenuEvent(QContextMenuEvent *event) override
    {
      RepoView *view = RepoView::parentView(this);
      FileContextMenu menu(view, {mPatch.name()}, mDiff.index());
      menu.exec(event->globalPos());
    }

  private:
    void updateCheckState()
    {
      bool disabled = false;
      Qt::CheckState state = Qt::Unchecked;
      switch (mDiff.index().isStaged(mPatch.name())) {
        case git::Index::Disabled:
          disabled = true;
          break;

        case git::Index::Unstaged:
          break;

        case git::Index::PartiallyStaged:
          state = Qt::PartiallyChecked;
          break;

        case git::Index::Staged:
          state = Qt::Checked;
          break;

        case git::Index::Conflicted:
          disabled = (mPatch.count() > 0);
          break;
      }

      mCheck->setCheckState(state);
      mCheck->setEnabled(!disabled);
    }

    git::Diff mDiff;
    git::Patch mPatch;

    QCheckBox *mCheck;
    QToolButton *mLfsButton = nullptr;
    EditButton *mEdit;
    DisclosureButton *mDisclosureButton;
  };

  FileWidget(
    DiffView *view,
    const git::Diff &diff,
    const git::Patch &patch,
    const git::Patch &staged,
    QWidget *parent = nullptr)
    : QWidget(parent), mView(view), mDiff(diff), mPatch(patch)
  {
    setObjectName("FileWidget");
    QVBoxLayout *layout = new QVBoxLayout(this);

    git::Repository repo = RepoView::parentView(this)->repo();

    QString name = patch.name();
    QString path = repo.workdir().filePath(name);
    bool submodule = repo.lookupSubmodule(name).isValid();

    bool binary = patch.isBinary();
    if (patch.isUntracked()) {
      QFile dev(path);
      if (dev.open(QFile::ReadOnly)) {
        QByteArray content = dev.readAll();
        git::Buffer buffer(content.constData(), content.length());
        binary = buffer.isBinary();
      }
    }

    bool lfs = patch.isLfsPointer();

    mHeader = new Header(diff, patch, binary, lfs, submodule, parent);
    layout->addWidget(mHeader);

    DisclosureButton *disclosureButton = mHeader->disclosureButton();
    connect(disclosureButton, &DisclosureButton::toggled, [this](bool visible) {

      if (mHeader->lfsButton() && !visible) {
        mHunks.first()->setVisible(false);
        if (!mImages.isEmpty())
          mImages.first()->setVisible(false);
        return;
      }

      if (mHeader->lfsButton() && visible) {
        bool checked = mHeader->lfsButton()->isChecked();
        mHunks.first()->setVisible(!checked);
        if (!mImages.isEmpty())
          mImages.first()->setVisible(checked);
        return;
      }

      foreach (HunkWidget *hunk, mHunks)
        hunk->setVisible(visible);
    });

    if (diff.isStatusDiff()) {
      // Collapse on check.
      connect(mHeader->check(), &QCheckBox::stateChanged, [this](int state) {
        mHeader->disclosureButton()->setChecked(state != Qt::Checked);
        if (state != Qt::PartiallyChecked) {
          foreach (HunkWidget *hunk, mHunks)
            hunk->header()->check()->setChecked(state == Qt::Checked);
        }
      });
    }

    // Try to load an image from the file.
    if (binary) {
      layout->addWidget(addImage(disclosureButton, mPatch));
      return;
    }

    // Add untracked file content.
    if (patch.isUntracked()) {
      if (!QFileInfo(path).isDir())
        layout->addWidget(addHunk(diff, patch, -1, lfs, submodule));
      return;
    }

    // Generate a diff between the head tree and index.
    QSet<int> stagedHunks;
    if (staged.isValid()) {
      for (int i = 0; i < staged.count(); ++i)
        stagedHunks.insert(staged.lineNumber(i, 0, git::Diff::OldFile));
    }

    // Add diff hunks.
    int hunkCount = patch.count();
    for (int hidx = 0; hidx < hunkCount; ++hidx) {
      HunkWidget *hunk = addHunk(diff, patch, hidx, lfs, submodule);
      int startLine = patch.lineNumber(hidx, 0, git::Diff::OldFile);
      hunk->header()->check()->setChecked(stagedHunks.contains(startLine));
      layout->addWidget(hunk);
    }

    // LFS
    if (QToolButton *lfsButton = mHeader->lfsButton()) {
      connect(lfsButton, &QToolButton::clicked,
      [this, layout, disclosureButton, lfsButton](bool checked) {
        lfsButton->setText(checked ? tr("Show Pointer") : tr("Show Object"));
        mHunks.first()->setVisible(!checked);

        // Image already loaded.
        if (!mImages.isEmpty()) {
          mImages.first()->setVisible(checked);
          return;
        }

        // Load image.
        layout->addWidget(addImage(disclosureButton, mPatch, true));
      });
    }

    // Start hidden when the file is checked.
    bool expand = (mHeader->check()->checkState() == Qt::Unchecked);

    if (Settings::instance()->value("collapse/added").toBool() == true &&
        patch.status() == GIT_DELTA_ADDED)
      expand = false;

    if (Settings::instance()->value("collapse/deleted").toBool() == true &&
        patch.status() == GIT_DELTA_DELETED)
      expand = false;

    disclosureButton->setChecked(expand);
  }

  bool isEmpty()
  {
    return (mHunks.isEmpty() && mImages.isEmpty());
  }

  Header *header() const { return mHeader; }

  QString name() const { return mPatch.name(); }

  QList<HunkWidget *> hunks() const { return mHunks; }

  QWidget *addImage(
    DisclosureButton *button,
    const git::Patch patch,
    bool lfs = false)
  {
    Images *images = new Images(patch, lfs, this);

    // Hide on file collapse.
    if (!lfs)
      connect(button, &DisclosureButton::toggled, images, &QLabel::setVisible);

    // Remember image.
    mImages.append(images);

    return images;
  }

  HunkWidget *addHunk(
    const git::Diff &diff,
    const git::Patch &patch,
    int index,
    bool lfs,
    bool submodule)
  {
    HunkWidget *hunk =
      new HunkWidget(mView, diff, patch, index, lfs, submodule, this);

    // Respond to check box click.
    QCheckBox *check = hunk->header()->check();
    check->setVisible(diff.isStatusDiff() && !submodule && !patch.isConflicted());
    connect(check, &QCheckBox::clicked, this, &FileWidget::stageHunks);

    // Respond to editor diagnostic signal.
    connect(hunk->editor(false), &TextEditor::diagnosticAdded,
    [this](int line, const TextEditor::Diagnostic &diag) {
      emit diagnosticAdded(diag.kind);
    });

    // Remember hunk.
    mHunks.append(hunk);

    return hunk;
  }

  void stageHunks()
  {
    QBitArray hunks(mHunks.size());
    for (int i = 0; i < mHunks.size(); ++i)
      hunks[i] = mHunks.at(i)->header()->check()->isChecked();

    git::Index index = mDiff.index();
    if (hunks == QBitArray(hunks.size(), true)) {
      index.setStaged({mPatch.name()}, true);
      return;
    }

    if (hunks == QBitArray(hunks.size(), false)) {
      index.setStaged({mPatch.name()}, false);
      return;
    }

    QByteArray buffer = mPatch.apply(hunks);
    if (buffer.isEmpty())
      return;

    // Add the buffer to the index.
    index.add(mPatch.name(), buffer);
  }

signals:
  void diagnosticAdded(TextEditor::DiagnosticKind kind);

private:
  DiffView *mView;

  git::Diff mDiff;
  git::Patch mPatch;

  Header *mHeader;
  QList<QWidget *> mImages;
  QList<HunkWidget *> mHunks;
};

} // anon. namespace

DiffView::DiffView(const git::Repository &repo, QWidget *parent)
  : QScrollArea(parent)
{
  setStyleSheet(kStyleSheet);
  setAcceptDrops(true);
  setWidgetResizable(true);
  setFocusPolicy(Qt::NoFocus);
  setContextMenuPolicy(Qt::ActionsContextMenu);

  mPlugins = Plugin::plugins(repo);

  // Update comments.
  if (Repository *remote = RepoView::parentView(this)->remoteRepo()) {
    connect(remote->account(), &Account::commentsReady, this, [this, remote](
      Repository *repo,
      const QString &oid,
      const Account::CommitComments &comments)
    {
      if (repo != remote)
        return;

      RepoView *view = RepoView::parentView(this);
      QList<git::Commit> commits = view->commits();
      if (commits.size() != 1 || oid != commits.first().id().toString())
        return;

      mComments = comments;

      // Invalidate editors.
      foreach (QWidget *widget, mFiles) {
        foreach (HunkWidget *hunk, static_cast<FileWidget *>(widget)->hunks())
          hunk->invalidate();
      }

      // Load commit comments.
      if (!canFetchMore())
        fetchMore();
    });
  }
}

DiffView::~DiffView() {}

QWidget *DiffView::file(int index)
{
  fetchAll(index);
  return mFiles.at(index);
}

void DiffView::setDiff(const git::Diff &diff)
{
  RepoView *view = RepoView::parentView(this);
  git::Repository repo = view->repo();

  // Disconnect signals.
  foreach (QMetaObject::Connection connection, mConnections)
    disconnect(connection);
  mConnections.clear();

  // Clear state.
  mFiles.clear();
  mStagedPatches.clear();
  mComments = Account::CommitComments();

  // Set data.
  mDiff = diff;

  // Create a new widget.
  QWidget *widget = new QWidget(this);
  setWidget(widget);

  // Disable painting the background.
  // This allows drawing content over the border shadow.
  widget->setStyleSheet(".QWidget {background-color: transparent}");

  // Begin layout.
  QVBoxLayout *layout = new QVBoxLayout(widget);
  layout->setSpacing(4);
  layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

  if (!diff.isValid()) {
    if (repo.isHeadUnborn()) {
      QPushButton *button =
        new QPushButton(QIcon(":/file.png"), tr("Add new file"));
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      button->setStyleSheet("color: #484848");
      button->setIconSize(QSize(32, 32));
      button->setFlat(true);

      QFont buttonFont = button->font();
      buttonFont.setPointSize(24);
      button->setFont(buttonFont);

      // Open new editor associated with this view.
      connect(button, &QPushButton::clicked, view, &RepoView::newEditor);

      QLabel *label =
        new QLabel(tr("Or drag files here to copy into the repository"));
      label->setStyleSheet("color: #696969");
      label->setAlignment(Qt::AlignHCenter);

      QFont labelFont = label->font();
      labelFont.setPointSize(18);
      label->setFont(labelFont);

      layout->addStretch();
      layout->addWidget(button, 0, Qt::AlignHCenter);
      layout->addWidget(label, 0, Qt::AlignHCenter);
      layout->addStretch();
    }

    return;
  }

  // Generate a diff between the head tree and index.
  if (diff.isStatusDiff()) {
    if (git::Reference head = repo.head()) {
      if (git::Commit commit = head.target()) {
        git::Diff stagedDiff = repo.diffTreeToIndex(commit.tree());
        for (int i = 0; i < stagedDiff.count(); ++i)
          mStagedPatches[stagedDiff.name(i)] = stagedDiff.patch(i);
      }
    }
  }

  if (canFetchMore())
    fetchMore();

  // Load patches on demand.
  QScrollBar *scrollBar = verticalScrollBar();
  mConnections.append(
    connect(scrollBar, &QScrollBar::valueChanged, [this](int value) {
      if (value > verticalScrollBar()->maximum() / 2 && canFetchMore())
        fetchMore();
    })
  );

  mConnections.append(
    connect(scrollBar, &QScrollBar::rangeChanged, [this](int min, int max) {
      if (max - min < this->widget()->height() / 2 && canFetchMore())
        fetchMore();
    })
  );

  // Request comments for this diff.
  if (Repository *remoteRepo = view->remoteRepo()) {
    QList<git::Commit> commits = view->commits();
    if (commits.size() == 1) {
      QString oid = commits.first().id().toString();
      remoteRepo->account()->requestComments(remoteRepo, oid);
    }
  }
}

bool DiffView::scrollToFile(int index)
{
  // Ensure that the given index is loaded.
  fetchAll(index);

  // Finish layout by processing events. May cause a new diff to
  // be loaded. In that case the scroll widget will be different.
  QWidget *ptr = widget();
  QCoreApplication::processEvents();
  if (widget() != ptr)
    return false;

  // Scroll to the widget.
  verticalScrollBar()->setValue(mFiles.at(index)->y());
  return true;
}

void DiffView::setFilter(const QStringList &paths)
{
  fetchAll();
  QSet<QString> set = QSet<QString>::fromList(paths);
  foreach (QWidget *widget, mFiles) {
    FileWidget *file = static_cast<FileWidget *>(widget);
    file->setVisible(set.isEmpty() || set.contains(file->name()));
  }
}

QList<TextEditor *> DiffView::editors()
{
  fetchAll();
  QList<TextEditor *> editors;
  foreach (QWidget *widget, mFiles) {
    foreach (HunkWidget *hunk, static_cast<FileWidget *>(widget)->hunks())
      editors.append(hunk->editor());
  }

  return editors;
}

void DiffView::ensureVisible(TextEditor *editor, int pos)
{
  HunkWidget *hunk = static_cast<HunkWidget *>(editor->parentWidget());
  hunk->header()->button()->setChecked(true);

  FileWidget *file = static_cast<FileWidget *>(hunk->parentWidget());
  file->header()->disclosureButton()->setChecked(true);

  int fileY = hunk->parentWidget()->y();
  int y = fileY + hunk->y() + editor->y() + editor->pointFromPosition(pos).y();

  QScrollBar *scrollBar = verticalScrollBar();
  int val = scrollBar->value();
  int step = scrollBar->pageStep();
  if (y < val) {
    scrollBar->setValue(y);
  } else if (y >= val + step) {
    scrollBar->setValue(y - step + editor->textHeight(0));
  }
}

void DiffView::dropEvent(QDropEvent *event)
{
  if (event->dropAction() != Qt::CopyAction)
    return;

  event->acceptProposedAction();

  // Copy files into the workdir.
  RepoView *view = RepoView::parentView(this);
  git::Repository repo = view->repo();
  foreach (const QUrl &url, event->mimeData()->urls()) {
    if (url.isLocalFile())
      copy(url.toString(kUrlFormat), repo.workdir());
  }

  // FIXME: Work dir changed?
  view->refresh();
}

void DiffView::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

bool DiffView::canFetchMore()
{
  return (mDiff.isValid() && mFiles.size() < mDiff.count());
}

void DiffView::fetchMore()
{
  QVBoxLayout *layout = static_cast<QVBoxLayout *>(widget()->layout());

  // Add widgets.
  int init = mFiles.size();
  int patchCount = mDiff.count();
  RepoView *view = RepoView::parentView(this);
  for (int pidx = init; pidx < patchCount && pidx - init < 8; ++pidx) {
    git::Patch patch = mDiff.patch(pidx);
    if (!patch.isValid()) {
      // This diff is stale. Refresh the view.
      QTimer::singleShot(0, view, &RepoView::refresh);
      return;
    }

    git::Patch staged = mStagedPatches.value(patch.name());
    FileWidget *file = new FileWidget(this, mDiff, patch, staged, widget());
    layout->addWidget(file);

    mFiles.append(file);

    if (file->isEmpty()) {
      DisclosureButton *button = file->header()->disclosureButton();
      button->setChecked(false);
      button->setEnabled(false);
    }

    // Respond to diagnostic signal.
    connect(file, &FileWidget::diagnosticAdded,
            this, &DiffView::diagnosticAdded);
  }

  // Finish layout.
  if (mFiles.size() == mDiff.count()) {
    // Add comments widget.
    if (!mComments.comments.isEmpty())
      layout->addWidget(new CommentWidget(mComments.comments, widget()));

    layout->addStretch();
  }
}

void DiffView::fetchAll(int index)
{
  // Load all patches up to and including index.
  while ((index < 0 || mFiles.size() <= index) && canFetchMore())
    fetchMore();
}

#include "DiffView.moc"
