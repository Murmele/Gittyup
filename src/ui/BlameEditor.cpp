//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "BlameEditor.h"
#include "BlameMargin.h"
#include "FindWidget.h"
#include "MenuBar.h"
#include "RepoView.h"
#include "editor/TextEditor.h"
#include "git/Blame.h"
#include "git/Blob.h"
#include "git/Buffer.h"
#include "git/Commit.h"
#include "git/Index.h"
#include "git/Repository.h"
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QSaveFile>
#include <QShortcut>
#include <QSplitter>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {

const QString kSplitterKey = QString("blamesplitter");

class BlameCallbacks : public git::Blame::Callbacks {
public:
  BlameCallbacks() : mCanceled(false) {}

  void setCanceled(bool canceled) { mCanceled = canceled; }

  bool progress() override { return !mCanceled; }

private:
  bool mCanceled;
};

} // namespace

BlameEditor::BlameEditor(const git::Repository &repo, QWidget *parent)
    : QWidget(parent), mRepo(repo), mCallbacks(new BlameCallbacks) {
  // Create editor.
  mEditor = new TextEditor(this);
  connect(mEditor, &TextEditor::linesAdded, this,
          &BlameEditor::adjustLineMarginWidth);
  connect(mEditor, &TextEditor::settingsChanged, this,
          &BlameEditor::adjustLineMarginWidth);

  // Create blame margin.
  mMargin = new BlameMargin(mEditor, this);
  connect(mMargin, &BlameMargin::linkActivated, this,
          &BlameEditor::linkActivated);

  // Add find widget.
  mFind = new FindWidget(this, this);
  mFind->hide(); // Start hidden.

  // Add widgets.
  QSplitter *splitter = new QSplitter(this);
  splitter->setHandleWidth(1);
  splitter->addWidget(mEditor);
  splitter->addWidget(mMargin);
  splitter->setStretchFactor(0, 1);
  connect(splitter, &QSplitter::splitterMoved, this, [splitter] {
    QSettings().setValue(kSplitterKey, splitter->saveState());
  });

  // Restore splitter state.
  splitter->restoreState(QSettings().value(kSplitterKey).toByteArray());

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(mFind);
  layout->addWidget(splitter, 1);

  // Handle asynchronous blame termination.
  connect(&mBlame, &QFutureWatcher<git::Blame>::finished, [this] {
    QFuture<git::Blame> future = mBlame.future();
    if (future.resultCount() > 0) {
      git::Blame blame = future.result();
      mMargin->setBlame(mRepo, blame);
      mMargin->setVisible(blame.isValid());
    }
  });

  // Margin starts hidden by default.
  mMargin->setVisible(false);
}

QString BlameEditor::name() const {
  return !mName.isEmpty() ? mName : tr("Untitled");
}

QString BlameEditor::path() const {
  if (mName.isEmpty())
    return QString();

  bool abs = QDir::isAbsolutePath(mName);
  Q_ASSERT(abs || mRepo.isValid());

  return abs ? mName : mRepo.workdir().filePath(mName);
}

QString BlameEditor::revision() const {
  return !mRevision.isEmpty() ? mRevision : tr("Not Tracked");
}

bool BlameEditor::load(const QString &name, const git::Blob &blob,
                       const git::Commit &commit) {
  // Clear content.
  clear();

  // Remember name.
  mName = name;

  // Load content.
  QByteArray content;
  if (blob.isValid()) {
    if (blob.isBinary())
      return false;

    content = blob.content();
    mRevision = commit.isValid() ? commit.shortId() : tr("HEAD");

  } else {
    if (mRepo.isValid() && mRepo.index().isTracked(name))
      mRevision = tr("Working Copy");

    QFile file(path());
    if (!file.open(QFile::ReadOnly))
      return false;

    content = file.readAll();
    git::Buffer buffer(content.constData(), content.length());
    if (buffer.isBinary())
      return false;
  }

  // Set editor text.
  mEditor->setReadOnly(false);
  mEditor->load(name, mRepo.isValid() ? mRepo.decode(content) : content);
  mEditor->setReadOnly(blob.isValid());

  mMargin->setVisible(mRepo.isValid() && !content.isEmpty());

  // Calculate blame.
  if (mRepo.isValid() && !content.isEmpty()) {
    mMargin->startBlame(name);
    mBlame.setFuture(QtConcurrent::run(mRepo, &git::Repository::blame, name,
                                       commit, mCallbacks.data()));
  }

  return true;
}

void BlameEditor::cancelBlame() {
  BlameCallbacks *callbacks = static_cast<BlameCallbacks *>(mCallbacks.data());
  callbacks->setCanceled(true);
  if (mBlame.isRunning())
    mBlame.waitForFinished();
  mBlame.setFuture(QFuture<git::Blame>());
  callbacks->setCanceled(false);
}

void BlameEditor::save() {
  QString path = this->path();
  if (path.isEmpty()) {
    QDir dir = mRepo.isValid() ? mRepo.workdir() : QDir();
    path = QFileDialog::getSaveFileName(this, tr("Save File"), dir.path());
    if (path.isEmpty())
      return;

    // Set editor lexer.
    mEditor->setLexer(path);
    mEditor->startStyling(0);
  }

  QSaveFile file(path);
  if (!file.open(QFile::WriteOnly))
    return;

  QTextStream out(&file);
  if (mRepo.isValid())
    out.setCodec(mRepo.codec());

  out << mEditor->text();
  file.commit();

  mEditor->setSavePoint();

  // Remember the name.
  if (mName.isEmpty())
    mName = path;

  emit saved();
}

void BlameEditor::clear() {
  // Cancel find and blame.
  mFind->hide();
  cancelBlame();

  // Clear margin and editor.
  mMargin->clear();
  mMargin->setVisible(false);

  mEditor->setReadOnly(false);
  mEditor->clearAll();
  mEditor->setReadOnly(true);

  mName = QString();
  mRevision = QString();
}

void BlameEditor::find() {
  if (mEditor->length() > 0)
    mFind->showAndSetFocus();
}

void BlameEditor::findNext() { mFind->find(); }

void BlameEditor::findPrevious() { mFind->find(FindWidget::Backward); }

void BlameEditor::adjustLineMarginWidth() {
  // Enable dynamic line margin width by tracking document changes.
  QByteArray lines = QByteArray::number(static_cast<int>(mEditor->lineCount()));
  int width = mEditor->textWidth(STYLE_LINENUMBER, lines.constData());
  int marginWidth = (mEditor->length() > 0) ? width + 8 : 0;
  mEditor->setMarginWidthN(TextEditor::LineNumber, marginWidth);
}
