#ifndef TEMPLATEDIALOG_H
#define TEMPLATEDIALOG_H

#include "TemplateButton.h"

#include <QDialog>

class QPushButton;
class QLineEdit;
class QTextEdit;
class QListWidget;
class QDialogButtonBox;

class TemplateDialog : public QDialog {
  Q_OBJECT
public:
  TemplateDialog(QList<TemplateButton::Template> &templates,
                 QWidget *parent = nullptr);

private:
  void addTemplate();
  void removeTemplate();
  void moveTemplateUp();
  void moveTemplateDown();
  void applyTemplates();
  bool uniqueName(QString name);
  void checkName(QString name);
  void showTemplate(int idx);
  void importTemplates(QString filename = QStringLiteral(""));
  void exportTemplates(QString filename = QStringLiteral(""));

  QPushButton *mUp;   // moving template up
  QPushButton *mDown; // moving template down
  QListWidget *mTemplateList;
  QPushButton *mAdd;
  QPushButton *mRemove;
  QDialogButtonBox *mButtonBox;

  QLineEdit *mName;
  QTextEdit *mTemplate;

  QList<TemplateButton::Template> &mTemplates;
  QList<TemplateButton::Template> mNew;

  bool mSupress{false};

  friend class TestCommitMessageTemplate;
};

#endif // TEMPLATEDIALOG_H
