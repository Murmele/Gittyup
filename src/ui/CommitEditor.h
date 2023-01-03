#ifndef COMMITEDITOR_H
#define COMMITEDITOR_H

#include "git/Repository.h"
#include "git/Diff.h"

#include <QFrame>
#include <QTextCharFormat>

class QPushButton;
class QLabel;
class TemplateButton;
class TextEdit;

/*!
 * \brief The CommitEditor class
 * This widget contains the textedit element for entering the commit message,
 * the buttons for commiting, staging all and unstage all
 * If a rebase is ongoing, the rebase continue and rebase abort button is shown
 */
class CommitEditor : public QFrame {
  Q_OBJECT

public:
  CommitEditor(const git::Repository &repo, QWidget *parent = nullptr);
  void commit(bool force = false);
  void abortRebase();
  void continueRebase();
  bool isRebaseAbortVisible() const;
  bool isRebaseContinueVisible() const;
  bool isCommitEnabled() const;
  void stage();
  bool isStageEnabled() const;
  void unstage();
  bool isUnstageEnabled() const;
  QString createFileList(const QStringList &list, int maxFiles);
  void setMessage(const QStringList &list);
  void setMessage(const QString &message);
  QString message() const;
  void setDiff(const git::Diff &diff);

public slots:
  void applyTemplate(const QString &t, const QStringList &files);
  void applyTemplate(const QString &t);

private:
  void updateButtons(bool yieldFocus = true);

  git::Repository mRepo;
  git::Diff mDiff;

  QLabel *mStatus;
  TextEdit *mMessage;
  QPushButton *mStage;
  QPushButton *mUnstage;
  QPushButton *mCommit;
  QPushButton *mRebaseAbort;
  QPushButton *mRebaseContinue;
  QPushButton *mMergeAbort;
  TemplateButton *mTemplate;

  bool mEditorEmpty = true;
  bool mPopulate = true;

  QString mDictName;
  QString mDictPath;
  QString mUserDict;

  QTextCharFormat mSpellError;
  QTextCharFormat mSpellIgnore;
};

#endif // COMMITEDITOR_H
