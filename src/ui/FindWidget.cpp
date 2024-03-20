//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "FindWidget.h"
#include "MenuBar.h"
#include "editor/TextEditor.h"
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QShortcut>
#include <QShowEvent>
#include <QStyleOption>

namespace {

const QString kPrevButtonStyle = "QToolButton {"
                                 "  border-top-right-radius: 0px;"
                                 "  border-bottom-right-radius: 0px"
                                 "}";

const QString kNextButtonStyle = "QToolButton {"
                                 "  border-left: none;"
                                 "  border-top-left-radius: 0px;"
                                 "  border-bottom-left-radius: 0px"
                                 "}";

const QString kFieldStyle = "QLineEdit {"
                            "  border-radius: 4px;"
                            "  padding: 0px 4px 0px 4px"
                            "}";

} // namespace

QString FindWidget::sText;

FindWidget::SegmentedButton::SegmentedButton(QWidget *parent)
    : QWidget(parent), mPrev(nullptr), mNext(nullptr) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  mPrev = new Segment(Segment::Prev, this);
  mPrev->setObjectName("PreviousArrow");
  mPrev->setStyleSheet(kPrevButtonStyle);
  layout->addWidget(mPrev);

  mNext = new Segment(Segment::Next, this);
  mNext->setStyleSheet(kNextButtonStyle);
  layout->addWidget(mNext);
}

FindWidget::SegmentedButton::Segment::Segment(Kind kind, QWidget *parent)
    : QToolButton(parent), mKind(kind) {}

void FindWidget::SegmentedButton::Segment::paintEvent(QPaintEvent *event) {
  // Draw background.
  QToolButton::paintEvent(event);

  // Draw arrow.
  QPainterPath path;
  qreal size = 2.5;
  qreal x = width() / 2.0;
  qreal y = height() / 2.0;
  bool prev = (mKind == Prev);
  path.moveTo(x + (prev ? size : -size), y - size - 0.5);
  path.lineTo(x + (prev ? -size : size), y);
  path.lineTo(x + (prev ? size : -size), y + size + 0.5);

  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing);
  painter.setPen(QPen(painter.pen().color(), 1.2));
  painter.drawPath(path);
}

FindWidget::FindWidget(EditorProvider *provider, QWidget *parent)
    : QWidget(parent), mEditorProvider(provider) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(8);
  layout->addStretch();

  mHits = new QLabel(this);
  mHits->setVisible(false);
  layout->addWidget(mHits);

  mButtons = new SegmentedButton(this);
  mButtons->setEnabled(false);
  layout->addWidget(mButtons);

  mField = new QLineEdit(this);
  mField->setStyleSheet(kFieldStyle);
  mField->setClearButtonEnabled(true);
  mField->setPlaceholderText(tr("Search"));
  mField->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  layout->addWidget(mField);

  QToolButton *done = new QToolButton(this);
  done->setText(tr("Done"));
  layout->addWidget(done);

  // Adjust minimum height of all buttons.
  int height = mField->sizeHint().height();
  done->setMinimumHeight(height);
  mButtons->prev()->setMinimumHeight(height);
  mButtons->next()->setMinimumHeight(height);

  // Show hit count whenever text is not empty.
  connect(mField, &QLineEdit::textChanged, [this](const QString &text) {
    sText = text;
    highlightAll();

    if (MenuBar *menuBar = MenuBar::instance(this))
      menuBar->updateFind();
  });

  // Connect previous button.
  connect(mButtons->prev(), &QToolButton::clicked,
          [this]() { find(FindWidget::Backward); });

  // Connect return and next button.
  auto next = [this]() { find(); };
  connect(mButtons->next(), &QToolButton::clicked, next);
  connect(mField, &QLineEdit::returnPressed, next);

  // Connect hide actions.
  QShortcut *esc = new QShortcut(tr("Esc"), this);
  esc->setContext(Qt::WidgetWithChildrenShortcut);
  connect(esc, &QShortcut::activated, this, &FindWidget::hide);
  connect(done, &QToolButton::clicked, this, &FindWidget::hide);
}

void FindWidget::reset() { mEditorIndex = 0; }

void FindWidget::clearHighlights() {
  foreach (TextEditor *editor, mEditorProvider->editors())
    editor->clearHighlights();
}

void FindWidget::highlightAll() {
  int matches = 0;
  foreach (TextEditor *editor, mEditorProvider->editors())
    matches += editor->highlightAll(sText);

  QString text;
  switch (matches) {
    case 0:
      text = tr("Not found");
      break;

    case 1:
      text = tr("%1 match").arg(matches);
      break;

    default:
      text = tr("%1 matches").arg(matches);
      break;
  }

  mHits->setText(text);
  mHits->setVisible(!sText.isEmpty());
  mButtons->setEnabled(matches);

  // Go to the first match.
  if (matches)
    find(Forward);
}

void FindWidget::find(Direction direction) {
  bool forward = (direction != Backward);

  // Search through all editors until a match is found.
  // Then search the initial editor again from the beginning.
  QList<TextEditor *> editors = mEditorProvider->editors();
  for (int i = 0; i < editors.size() + 1; ++i) {
    TextEditor *editor = editors.at(mEditorIndex);

    // Advance to end of selection.
    if (direction == Advance) {
      int sel = editor->selectionEnd();
      editor->setSelection(sel, sel);
    }

    // Search without wrapping.
    int pos = editor->find(sText, forward, isVisible());
    if (pos >= 0) {
      // Scroll the match into view.
      mEditorProvider->ensureVisible(editor, pos);
      return;
    }

    // Choose next index.
    if (forward) {
      ++mEditorIndex;
      if (mEditorIndex > editors.size() - 1)
        mEditorIndex = 0;
    } else {
      --mEditorIndex;
      if (mEditorIndex < 0)
        mEditorIndex = editors.size() - 1;
    }

    // Reset current editor selection.
    editor->setSelection(0, 0);

    // Reset next editor selection.
    TextEditor *next = editors.at(mEditorIndex);
    int extreme = forward ? 0 : next->length();
    next->setSelection(extreme, extreme);
  }
}

void FindWidget::showAndSetFocus() {
  show();
  mField->setText(sText);
  mField->selectAll();
  mField->setFocus();
}

void FindWidget::paintEvent(QPaintEvent *) {
  QStyleOption opt;
  opt.initFrom(this);
  QPainter painter(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}

void FindWidget::hideEvent(QHideEvent *event) {
  QWidget::hideEvent(event);

  if (!event->spontaneous())
    clearHighlights();
}

void FindWidget::showEvent(QShowEvent *event) {
  if (!event->spontaneous() && !sText.isEmpty())
    highlightAll();

  QWidget::showEvent(event);
}
