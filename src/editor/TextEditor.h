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
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <QMenu>
#include <ILexer.h>
#include <LexerModule.h>
#include <CatalogueModules.h>
#include <SciLexer.h>
#include <ScintillaEdit.h>
#include <ScintillaQt.h>
#include <Platform.h>

class TextEditor : public ScintillaEdit {
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

  enum MenuAction {
    None,
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Delete,
    StageSelected,
    UnstageSelected,
    DiscardSelected,
    SelectAll,
  };
  Q_ENUM(MenuAction);

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

  // Make wheel event public.
  // FIXME: This should be an event filter?
  void wheelEvent(QWheelEvent *event) override {
    ScintillaEdit::wheelEvent(event);
  }
  void keyPressEvent(QKeyEvent *ke) override;

  QRect textRectangle() const {
    // TODO: Port to scintilla 5.x
    // Prior code used Editor::GetTextRectangle, but that's not accessible here
    // with Scintilla 5.x. This is sometimes (but not always!) a good
    // approximate
    QRect rc = contentsRect();
    rc.adjust(0, 0, -marginRight(), 0);
    return rc;
  };

  QFont styleGetQFont(int style);

  QPoint pointFromPosition(int pos);

  void showEvent(QShowEvent *event) override {
    QAbstractScrollArea::showEvent(event);
    emit onVisible();
  }

signals:
  void onVisible();
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
  void Command(MenuAction action);
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  int diagnosticMarker(int line);
  void loadMarkerIcon(Marker marker, const QIcon &icon);
  void loadMarkerPixmap(Marker marker, const QPixmap &pixmap);
  void AddToPopUp(const QString &label, MenuAction cmd = None,
                  bool enabled = true);
  void StyleSetQFont(int style, const QFont &font);
  void markerDefineImage(int markerNumber, const QImage &image);
  void applyLexerStyles();

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
  QMenu mPopup;
};

#endif
