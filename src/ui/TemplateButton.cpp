#include "TemplateButton.h"

#include "TemplateDialog.h"

#include <QMenu>
#include <QTranslator>
#include <QSettings>

namespace {
const QString configTemplate = "Configure templates"; // TODO: translation
const QString kTemplatesKey = "templates";
const QString separator = ":";
} // namespace

const QString TemplateButton::cursorPositionString = QStringLiteral("%|");
// Showing the position where the files shall be placed
// The number regex dertermines how many filenames shall be shown and if
// there are more it will be replaced by ...
const QString TemplateButton::filesPosition =
    QStringLiteral("${files:([0-9]*)}");

TemplateButton::TemplateButton(QWidget *parent) : QToolButton(parent) {
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

const QList<TemplateButton::Template> &TemplateButton::templates() {
  return mTemplates;
}

void TemplateButton::storeTemplates() {
  QSettings settings;
  settings.beginGroup(kTemplatesKey);

  // delete old templates
  QList<TemplateButton::Template> old = loadTemplates();
  for (auto templ : old)
    settings.remove(templ.name);

  for (auto templ : mTemplates) {
    QString value = templ.value;
    value = value.replace("\n", "\\n");
    value = value.replace("\t", "\\t");
    settings.setValue(templ.name, value);
  }
  settings.endGroup();
}

QList<TemplateButton::Template> TemplateButton::loadTemplates() {
  QSettings settings;
  settings.beginGroup(kTemplatesKey);

  QStringList list = settings.allKeys();
  QList<TemplateButton::Template> templates;

  for (auto templateName : list) {
    QString value = settings.value(templateName).toString();
    value = value.replace("\\n", "\n");
    value = value.replace("\\t", "\t");
    Template t;
    t.name = templateName;
    t.value = value;
    templates.append(t);
  }

  settings.endGroup();
  return templates;
}
