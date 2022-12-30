#ifndef TEMPLATEBUTTON_H
#define TEMPLATEBUTTON_H

#include "git/Config.h"

#include <QToolButton>

class TemplateButton : public QToolButton {
  Q_OBJECT
public:
  struct Template {
    QString name{""};
    QString value{""};
  };
  static const QString cursorPositionString;
  static const QString filesPosition;

  TemplateButton(git::Config config, QWidget *parent = nullptr);
  QMenu *menu() const;
  void showMenu();
  void storeTemplates();
  QList<Template> loadTemplates();
  void updateMenu();
  const QList<Template> &templates();
signals:
  void templateChanged(QString &str);

private:
  void actionTriggered(QAction *action);

  QMenu *mMenu{nullptr};
  QList<Template> mTemplates;
  git::Config mConfig;
};

#endif
