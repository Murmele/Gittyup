//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef FINDWIDGET_H
#define FINDWIDGET_H

#include <QToolButton>
#include <QWidget>

class TextEditor;
class QLabel;
class QLineEdit;

class EditorProvider {
public:
  virtual QList<TextEditor *> editors() = 0;
  virtual void ensureVisible(TextEditor *editor, int pos) = 0;
};

class FindWidget : public QWidget {
  Q_OBJECT

public:
  // The difference between Forward and Advance is that Forward doesn't
  // advance if the current selection already matches the search term.
  enum Direction { Backward, Forward, Advance };

  FindWidget(EditorProvider *provider, QWidget *parent = nullptr);

  void reset();

  static QString text() { return sText; }
  static void setText(const QString &text) { sText = text; }

  void clearHighlights();
  void highlightAll();
  void find(Direction direction = Advance);

  void showAndSetFocus();

protected:
  void paintEvent(QPaintEvent *event) override;
  void hideEvent(QHideEvent *event) override;
  void showEvent(QShowEvent *event) override;

private:
  class SegmentedButton : public QWidget {
  public:
    SegmentedButton(QWidget *parent = nullptr);

    QToolButton *prev() const { return mPrev; }
    QToolButton *next() const { return mNext; }

  private:
    class Segment : public QToolButton {
    public:
      enum Kind { Prev, Next };

      Segment(Kind kind, QWidget *parent = nullptr);

    protected:
      void paintEvent(QPaintEvent *event) override;

    private:
      Kind mKind;
    };

    Segment *mPrev;
    Segment *mNext;
  };

  int mEditorIndex = 0;
  EditorProvider *mEditorProvider;

  QLabel *mHits;
  QLineEdit *mField;
  SegmentedButton *mButtons;

  static QString sText;
};

#endif
