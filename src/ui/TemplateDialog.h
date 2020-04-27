#ifndef TEMPLATEDIALOG_H
#define TEMPLATEDIALOG_H

#include "TemplateButton.h"

#include <QDialog>

class QPushButton;
class QLineEdit;
class QTextEdit;
class QListWidget;
class QDialogButtonBox;

class TemplateDialog: public QDialog {
    Q_OBJECT
public:
    TemplateDialog(QList<TemplateButton::Template>& templates, QWidget *parent);
private:
    void addTemplate();
    void removeTemplate();
    void moveTemplateUp();
    void moveTemplateDown();
    void applyTemplates();
    bool uniqueName(QString name);
    void checkName(QString name);
    void showTemplate(int idx);

    QPushButton* mUp; // moving template up
    QPushButton* mDown; // moving template down
    QListWidget* mTemplateList;
    QPushButton* mAdd;
    QPushButton* mRemove;
    QDialogButtonBox* mButtonBox;

    QLineEdit* mName;
    QTextEdit* mTemplate;

    QList<TemplateButton::Template> &mTemplates;
    QList<TemplateButton::Template> mNew;

    bool mSupress{false};
};

#endif // TEMPLATEDIALOG_H
