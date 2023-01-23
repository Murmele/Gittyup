#include "AmendDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTimeEdit>
#include <QRadioButton>
#include <QGroupBox>

enum Row { Author = 0, Committer, CommitMessageLabel, CommitMessage, Buttons };

class DateSelectionGroupWidget : public QGroupBox {
  Q_OBJECT
public:
  DateSelectionGroupWidget(QWidget *parent = nullptr)
      : QGroupBox(tr("Datetime source"), parent) {
    QHBoxLayout *l = new QHBoxLayout();

    current = new QRadioButton(tr("Current"), this);
    current->setObjectName("Current");
    manual = new QRadioButton(tr("Manual"), this);
    manual->setObjectName("Manual");
    original = new QRadioButton(tr("Original"), this);
    original->setObjectName("Original");

    current->setChecked(true);

    connect(current, &QRadioButton::clicked,
            [this]() { emit typeChanged(type()); });
    connect(manual, &QRadioButton::clicked,
            [this]() { emit typeChanged(type()); });
    connect(original, &QRadioButton::clicked,
            [this]() { emit typeChanged(type()); });

    l->addWidget(current);
    l->addWidget(manual);
    l->addWidget(original);
    setLayout(l);
  }
  ContributorInfo::SelectedDateTimeType type() {
    if (original->isChecked()) {
      return ContributorInfo::SelectedDateTimeType::Original;
    } else if (manual->isChecked()) {
      return ContributorInfo::SelectedDateTimeType::Manual;
    }
    return ContributorInfo::SelectedDateTimeType::Current;
  }

signals:
  void typeChanged(ContributorInfo::SelectedDateTimeType);

private:
  QRadioButton *current;
  QRadioButton *manual;
  QRadioButton *original;
};

class InfoBox : public QGroupBox {
  Q_OBJECT
public:
  enum LocalRow { Name = 0, Email, DateType, CommitDate };

  InfoBox(const QString &title, const git::Signature &signature,
          QWidget *parent = nullptr)
      : QGroupBox(title + ":", parent), m_signature(signature) {

    setObjectName(title);

    auto *l = new QVBoxLayout();

    auto *lName = new QLabel(tr("Name:"), this);
    m_name = new QLineEdit(signature.name(), this);
    m_name->setObjectName("Name");
    auto *hName = new QHBoxLayout();
    hName->addWidget(lName);
    hName->addWidget(m_name);

    auto *lEmail = new QLabel(tr("Email:"), this);
    m_email = new QLineEdit(signature.email(), this);
    m_email->setObjectName("Email");
    auto *hEmail = new QHBoxLayout();
    hEmail->addWidget(lEmail);
    hEmail->addWidget(m_email);

    m_commitDateType = new DateSelectionGroupWidget(this);
    m_commitDateType->setObjectName(title + "CommitDateType");
    m_lCommitDate = new QLabel(tr("Commit date:"), this);
    m_commitDate = new QDateTimeEdit(signature.date().toLocalTime(), this);
    m_commitDate->setObjectName(title + "CommitDate");
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_commitDate->setSizePolicy(sp);
    auto *hDate = new QHBoxLayout();
    hDate->addWidget(m_lCommitDate);
    hDate->addWidget(m_commitDate);

    l->addLayout(hName);
    l->addLayout(hEmail);
    l->addWidget(m_commitDateType);
    l->addLayout(hDate);

    setLayout(l);

    dateTimeTypeChanged(m_commitDateType->type());

    connect(m_commitDateType, &DateSelectionGroupWidget::typeChanged, this,
            &InfoBox::dateTimeTypeChanged);
  }

  ContributorInfo getInfo() const {
    ContributorInfo ci;
    ci.name = name();
    ci.email = email();
    ci.commitDate = commitDate();
    ci.commitDateType = commitDateType();

    return ci;
  }

private slots:
  void dateTimeTypeChanged(const ContributorInfo::SelectedDateTimeType type) {
    const auto enabled = type == ContributorInfo::SelectedDateTimeType::Manual;
    m_lCommitDate->setVisible(enabled);
    m_commitDate->setVisible(enabled);
  }

private:
  QString name() const { return m_name->text(); }

  QString email() const { return m_email->text(); }

  QDateTime commitDate() const {
    if (commitDateType() == ContributorInfo::SelectedDateTimeType::Original) {
      return m_signature.date().toLocalTime();
    } else {
      return m_commitDate->dateTime();
    }
  }

  ContributorInfo::SelectedDateTimeType commitDateType() const {
    return m_commitDateType->type();
  }

  QLineEdit *m_name;
  QLineEdit *m_email;
  QDateTimeEdit *m_commitDate;
  QLabel *m_lCommitDate;
  DateSelectionGroupWidget *m_commitDateType;

  git::Signature m_signature;
};

AmendDialog::AmendDialog(const git::Signature &author,
                         const git::Signature &committer,
                         const QString &commitMessage, QWidget *parent)
    : QDialog(parent) {

  auto *l = new QGridLayout();

  // author
  // committer
  // message

  m_authorInfo = new InfoBox(tr("Author"), author, this);
  l->addWidget(m_authorInfo, Row::Author, 0, 1, 2);

  m_committerInfo = new InfoBox(tr("Committer"), committer, this);
  l->addWidget(m_committerInfo, Row::Committer, 0, 1, 2);

  auto *lMessage = new QLabel(tr("Commit Message:"), this);
  m_commitMessage = new QTextEdit(commitMessage, this);
  m_commitMessage->setObjectName("Textlabel Commit Message");
  l->addWidget(lMessage, Row::CommitMessageLabel, 0);
  l->addWidget(m_commitMessage, Row::CommitMessage, 0, 1, 2);

  auto *ok = new QPushButton(tr("Amend"), this);
  auto *cancel = new QPushButton(tr("Cancel"), this);

  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  auto *hl = new QHBoxLayout();
  hl->addWidget(cancel);
  hl->addWidget(ok);

  l->addLayout(hl, Buttons, 1);

  setLayout(l);
}

AmendInfo AmendDialog::getInfo() const {
  AmendInfo ai;
  ai.authorInfo = m_authorInfo->getInfo();
  ai.committerInfo = m_committerInfo->getInfo();
  ai.commitMessage = commitMessage();

  return ai;
}

QString AmendDialog::commitMessage() const {
  return m_commitMessage->toPlainText();
}

#include "AmendDialog.moc"
