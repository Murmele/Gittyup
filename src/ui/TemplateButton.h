#ifndef TEMPLATEBUTTON_H
#define TEMPLATEBUTTON_H

#include "git/Config.h"

#include <QToolButton>

class TemplateButton: public QToolButton {
    Q_OBJECT
public:
    typedef struct Template {
        QString name{""};
        QString value{""};
    };
    TemplateButton(git::Config config, QWidget* parent = nullptr);
    QMenu* menu() const;
    void showMenu();
    void storeTemplates();
    QList<Template> loadTemplates();
    void updateMenu();
signals:
    void templateChanged(const QString& str);
private:
    void actionTriggered(QAction* action);

    QMenu* mMenu{nullptr};
    QList<Template> mTemplates;
    git::Config mConfig;
};

#endif
