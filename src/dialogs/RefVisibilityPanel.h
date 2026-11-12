#ifndef REFVISIBILITYPANEL_H
#define REFVISIBILITYPANEL_H

#include "git/Repository.h"
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;

class RefVisibilityPanel : public QWidget {
  Q_OBJECT

public:
  RefVisibilityPanel(const git::Repository &repo = git::Repository(),
                     QWidget *parent = nullptr);

private:
  QStringList editedPatterns() const;
  void load();
  void updatePreview();
  void apply();
  void reset();
  void reportError(const QString &error);

  git::Repository mRepo;
  QCheckBox *mInherit = nullptr;
  QLabel *mOrigin;
  QPlainTextEdit *mRules;
  QLineEdit *mSearch;
  QListWidget *mRefs;
  QLabel *mSummary;
  QPushButton *mApply;
  QPushButton *mShowAll;
  bool mDirty = false;
};

#endif
