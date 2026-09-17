#ifndef DIFFVIEW_HUNKTEXTEDITOR_H
#define DIFFVIEW_HUNKTEXTEDITOR_H

#include "editor/TextEditor.h"

class HunkTextEditor : public TextEditor {
public:
  HunkTextEditor(QWidget *parent = nullptr) : TextEditor(parent) {}

protected:
  void focusOutEvent(QFocusEvent *event) override {
    if (event->reason() != Qt::PopupFocusReason)
      clearSelections();

    TextEditor::focusOutEvent(event);
  }
};

#endif // DIFFVIEW_HUNKTEXTEDITOR_H
