#include "StateAction.h"

StateAction::StateAction(const QString activeText, const QString inactiveText, QObject *parent):
        QAction(parent),
        mActiveText(activeText),
        mInactiveText(inactiveText)
{
  connect(this, &QAction::triggered, [this] (bool checked) {toggleActive(); emit triggered(checked);});
  setActive(mActive);
}

bool StateAction::isActive()
{
    return mActive;
}

void StateAction::setActive(bool active)
{
  mActive = active;
  if (mActive)
      setText(mActiveText);
  else
      setText(mInactiveText);

  if (isCheckable())
      setChecked(active);
  else
      setChecked(false);
}

void StateAction::toggleActive()
{
    setActive(!mActive);
}
