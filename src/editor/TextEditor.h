//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

// Do not reorder.
#include <cstddef>
#include <ILexer.h>
#include <LexerModule.h>
#include <Catalogue.h>
#include <SciLexer.h>
#include "ScintillaIFace.h"
#include <Platform.h>

class TextEditor : public Scintilla::ScintillaIFace {
  Q_OBJECT

public:
  enum Margin {
    Staged, // indicates staged or not
    LineNumber,
    LineNumbers,
    ErrorMargin,
  };

  enum Marker {
    Context,
    Ours,
    Theirs,
    Addition,
    Deletion,
    NoteMarker,
    WarningMarker,
    ErrorMarker,
    StagedMarker,
    UnstagedMarker,
    DiscardMarker,
  };

  enum Indicator {
    FindAll = INDIC_CONTAINER,
    FindCurrent,
    WordAddition,
    WordDeletion,
    NoteIndicator,
    WarningIndicator,
    ErrorIndicator
  };

  enum Style {
    EofNewline = STYLE_MAX - 4,
    CommentBody,
    CommentAuthor,
    CommentTimestamp
  };

  enum DiagnosticKind { Note, Warning, Error };

  enum {
    stageSelected = 30,
    unstageSelected = 31,
    discardSelected = 32,
  };

  struct Range {
    int pos;
    int len;
  };

  struct Diagnostic {
    DiagnosticKind kind;
    QString message;
    QString description;

    Range range;
    QString replacement;
  };

  TextEditor(QWidget *parent = nullptr);

  void applySettings();

  QString lexer() const;

  void setLineCount(int lines);
  void setLexer(const QString &path);
  void load(const QString &path, const QString &text);
  /*!
   * sets the statusDiff flag
   * \brief setStatusDiff
   * \param statusDiff See \variable mStatusDiff for more information
   */
  void setStatusDiff(bool statusDiff);

  void clearHighlights();
  int highlightAll(const QString &text);
  int find(const QString &text, bool forward = true, bool indicator = true);

  QList<Diagnostic> diagnostics(int line);
  void addDiagnostic(int line, const Diagnostic &diag);
  sptr_t WndProc(unsigned int message, uptr_t wParam, sptr_t lParam);

  // Make wheel event public.
  // FIXME: This should be an event filter?
  void wheelEvent(QWheelEvent *event) override {
    ScintillaIFace::wheelEvent(event);
  }
  void keyPressEvent(QKeyEvent *ke) override;

  QRect textRectangle() const {
    Scintilla::PRectangle pr = GetTextRectangle();
    return QRect(pr.left, pr.top, pr.Width(), pr.Height());
  }

signals:
  void settingsChanged();
  void highlightActivated(bool active);
  void diagnosticAdded(int line, const Diagnostic &diag);
  /*!
   * Emitted when in the context menu "stage selected" is triggered
   * \brief stageSelectedSignal
   * \param startPos Start line of selection
   * \param end End line of selection + 1
   */
  void stageSelectedSignal(int startPos, int end, bool emitSignal = true);
  /*!
   * Emitted when in the context menu "unstage selected" is triggered
   * \brief unstageSelectedSignal
   * \param startPos Start line of selection
   * \param end End line of selection + 1
   */
  void unstageSelectedSignal(int startPos, int end, bool emitSignal = true);
  /*!
   * Emitted when in the context menu "revert selected" is triggered
   * \brief discardSelectedSignal
   * \param startPos Start line of selection
   * \param end End line of selection + 1
   */
  void discardSelectedSignal(int startPos, int end);

protected:
  QSize viewportSizeHint() const override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void Command(int cmdId);

private:
  int diagnosticMarker(int line);
  void loadMarkerIcon(Marker marker, const QIcon &icon);
  void loadMarkerPixmap(Marker marker, const QPixmap &pixmap);
  void AddToPopUp(const char *label, int cmd = 0, bool enabled = true);
  void ContextMenu(Scintilla::Point pt);

  QString mPath;
  int mLineCount = -1;
  /*!
   * statusDiff Flag which determines if in the contextmenu stage actions are
   * shown or not Because when checking out commits, it should not possible to
   * select these actions. true: not commited diff. false: already commited
   * diff.
   */
  bool mStatusDiff{false};

  QColor mOursColor;
  QColor mTheirsColor;
  QColor mAdditionColor;
  QColor mDeletionColor;

  QIcon mNoteIcon;
  QIcon mWarningIcon;
  QIcon mErrorIcon;
  QPixmap mStagedIcon;
  QPixmap mUnStagedIcon;

  QMap<int, QList<Diagnostic>> mDiagnostics;
};

#endif
