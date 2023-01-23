#ifndef DIFFVIEW_EDITOR_H
#define DIFFVIEW_EDITOR_H

#include "editor/TextEditor.h"

class Editor : public TextEditor {
public:
  Editor(QWidget *parent = nullptr) : TextEditor(parent) {}

protected:
  void focusOutEvent(QFocusEvent *event) override {
    if (event->reason() != Qt::PopupFocusReason)
      clearSelections();

    TextEditor::focusOutEvent(event);
  }
};

#endif // DIFFVIEW_EDITOR_H
