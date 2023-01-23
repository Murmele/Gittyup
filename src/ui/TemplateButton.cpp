#include "TemplateButton.h"

#include "TemplateDialog.h"

#include <QMenu>
#include <QTranslator>

namespace {
QString configTemplate = "Configure templates"; // TODO: translation
const QString kTemplatesKey = "templates.template";
const QString kTemplatesRegexp = ".*_[0-9]*";
const QString separator = ":";
} // namespace

const QString TemplateButton::cursorPositionString = QStringLiteral("%|");

TemplateButton::TemplateButton(git::Config config, QWidget *parent)
    : QToolButton(parent), mConfig(config) {
  setPopupMode(QToolButton::InstantPopup);
  mMenu = new QMenu(this);

  mTemplates = loadTemplates();
  updateMenu();

  setMenu(mMenu);

  connect(this, &QToolButton::triggered, this,
          &TemplateButton::actionTriggered);
}

void TemplateButton::actionTriggered(QAction *action) {
  if (action->text() == configTemplate) {
    TemplateDialog dialog(mTemplates, this);
    if (dialog.exec()) {
      storeTemplates();
      updateMenu();
    }
    return;
  }

  for (auto templ : mTemplates) {
    if (templ.name == action->text()) {
      emit templateChanged(templ.value);
      break;
    }
  }
}

void TemplateButton::updateMenu() {
  mMenu->clear();

  for (auto templ : mTemplates)
    mMenu->addAction(templ.name);

  mMenu->addSeparator();
  mMenu->addAction(configTemplate);

  setMenu(mMenu);
}

void TemplateButton::storeTemplates() {
  /*
   * Workaround, because reading all variables in one backend is not possible
   * with this library. Cleaner would be to have: [Templates] T1 = value of
   * template 1 T2 = value of template 2
   *
   * Actual implementation (name and value are separated by a :):
   * [Templates]
   *  template = T1:value of template 1
   *  template = T2:value of template 2
   */

  // delete old templates
  QList<TemplateButton::Template> old = loadTemplates();
  for (auto templ : old)
    assert(mConfig.remove(kTemplatesKey, templ.name + separator + ".*"));

  for (auto templ : mTemplates) {
    QString value = templ.value;
    value = value.replace("\n", "\\n");
    value = value.replace("\t", "\\t");
    mConfig.setValue(kTemplatesKey, templ.name + separator + ".*",
                     templ.name + separator + value);
  }
}

QList<TemplateButton::Template> TemplateButton::loadTemplates() {
  QStringList list = mConfig.value(kTemplatesKey, "", QStringList());
  QList<TemplateButton::Template> templates;

  for (auto entry : list) {
    QStringList name_val = entry.split(separator);
    if (name_val.length() == 0)
      continue;

    QString name(name_val[0]);
    QString value;
    for (int i = 1; i < name_val.length(); i++) {
      value.append(name_val[i]);
      if (i < name_val.length() - 1)
        value.append(separator);
    }

    value = value.replace("\\n", "\n");
    value = value.replace("\\t", "\t");
    Template t;
    t.name = name;
    t.value = value;
    templates.append(t);
  }
  return templates;
}
