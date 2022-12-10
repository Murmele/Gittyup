#ifndef EDITBUTTON_H
#define EDITBUTTON_H

#include "Button.h"
#include "git/Patch.h"
#include <QMenu>

class EditButton : public Button {
  Q_OBJECT

public:
  EditButton(git::Patch &patch, int index, bool binary, bool lfs,
			 QWidget *parent = nullptr);

  void updatePatch(git::Patch &patch, int index, bool init = false);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QAction *mEditWorkingCopyAction;
  QAction *mEditNewRevisionAction;
  QAction *mEditOldRevisionAction;
};

#endif
