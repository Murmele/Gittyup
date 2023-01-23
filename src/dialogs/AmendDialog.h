#include <QDialog>
#include <QDateTime>
#include "git/Signature.h"

class QLineEdit;
class QTextEdit;
class QCheckBox;
class QDateTimeEdit;
class DateSelectionGroupWidget;
class QLabel;
class InfoBox;
class AmendDialog;

struct ContributorInfo {
  enum class SelectedDateTimeType { Current, Manual, Original };
  QString name;
  QString email;
  QDateTime commitDate;
  SelectedDateTimeType commitDateType;
};

struct AmendInfo {
  ContributorInfo authorInfo;
  ContributorInfo committerInfo;
  QString commitMessage;
};

class AmendDialog : public QDialog {
public:
  AmendDialog(const git::Signature &author, const git::Signature &committer,
              const QString &commitMessage, QWidget *parent = nullptr);

  AmendInfo getInfo() const;

private:
  QString commitMessage() const;

  InfoBox *m_authorInfo;
  InfoBox *m_committerInfo;
  QTextEdit *m_commitMessage;
};