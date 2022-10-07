#include "AmendDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QRadioButton>
#include <QGroupBox>

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
  AmendDialog::SelectedDateTimeType type() {
    if (original->isChecked()) {
      return AmendDialog::SelectedDateTimeType::Original;
    } else if (manual->isChecked()) {
      return AmendDialog::SelectedDateTimeType::Manual;
    }
    return AmendDialog::SelectedDateTimeType::Current;
  }

signals:
  void typeChanged(AmendDialog::SelectedDateTimeType);

private:
  QRadioButton *current;
  QRadioButton *manual;
  QRadioButton *original;
};

enum Row {
  AuthorName = 0,
  AuthorEmail,
  AuthorDateType,
  AuthorCommitDate,
  CommitterName,
  CommitterEmail,
  CommitterDateType,
  CommitterCommitDate,
  CommitMessage,

  Buttons
};

AmendDialog::AmendDialog(const git::Signature &author,
                         const git::Signature &committer,
                         const QString &commitMessage, QWidget *parent)
    : QDialog(parent), m_author(author), m_committer(committer) {

  auto *l = new QGridLayout();

  // author
  // committer
  // message

  auto *lAuthor = new QLabel(tr("Author name:"), this);
  m_authorName = new QLineEdit(author.name(), this);
  auto *lAuthorEmail = new QLabel(tr("Author email:"), this);
  m_authorEmail = new QLineEdit(author.email(), this);
  m_lAuthorCommitDate = new QLabel(tr("Author commit date:"), this);
  m_authorCommitDateType = new DateSelectionGroupWidget(this);
  m_authorCommitDateType->setObjectName("AuthorCommitDateType");
  m_authorCommitDate = new QDateTimeEdit(author.date().toLocalTime(), this);
  m_authorCommitDate->setObjectName("authorCommitDate");
  authorDateTimeTypeChanged(m_authorCommitDateType->type());
  l->addWidget(lAuthor, Row::AuthorName, 0);
  l->addWidget(m_authorName, Row::AuthorName, 1);
  l->addWidget(lAuthorEmail, Row::AuthorEmail, 0);
  l->addWidget(m_authorEmail, Row::AuthorEmail, 1);
  l->addWidget(m_authorCommitDateType, Row::AuthorDateType, 0, 1, 2);
  l->addWidget(m_lAuthorCommitDate, Row::AuthorCommitDate, 0);
  l->addWidget(m_authorCommitDate, Row::AuthorCommitDate, 1);

  auto *lCommitterName = new QLabel(tr("Committer name:"), this);
  m_committerName = new QLineEdit(committer.name(), this);
  auto *lCommitterEmail = new QLabel(tr("Committer email:"), this);
  m_committerEmail = new QLineEdit(committer.email(), this);
  m_lCommitterCommitDate = new QLabel(tr("Committer commit date:"), this);
  m_committerCommitDateType = new DateSelectionGroupWidget(this);
  m_committerCommitDateType->setObjectName("CommitterCommitDateType");
  m_committerCommitDate =
      new QDateTimeEdit(committer.date().toLocalTime(), this);
  m_committerCommitDate->setObjectName("committerCommitDate");
  committerDateTimeTypeChanged(m_committerCommitDateType->type());
  l->addWidget(lCommitterName, Row::CommitterName, 0);
  l->addWidget(m_committerName, Row::CommitterName, 1);
  l->addWidget(lCommitterEmail, Row::CommitterEmail, 0);
  l->addWidget(m_committerEmail, Row::CommitterEmail, 1);
  l->addWidget(m_committerCommitDateType, Row::CommitterDateType, 0, 1, 2);
  l->addWidget(m_lCommitterCommitDate, Row::CommitterCommitDate, 0);
  l->addWidget(m_committerCommitDate, Row::CommitterCommitDate, 1);

  auto *lMessage = new QLabel(tr("Message:"), this);
  m_commitMessage = new QTextEdit(commitMessage, this);
  l->addWidget(lMessage, Row::CommitMessage, 0);
  l->addWidget(m_commitMessage, Row::CommitMessage, 1, 2, 1);

  auto *ok = new QPushButton(tr("Amend"), this);
  auto *cancel = new QPushButton(tr("Cancel"), this);

  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  connect(m_authorCommitDateType, &DateSelectionGroupWidget::typeChanged, this,
          &AmendDialog::authorDateTimeTypeChanged);
  connect(m_committerCommitDateType, &DateSelectionGroupWidget::typeChanged,
          this, &AmendDialog::committerDateTimeTypeChanged);

  auto *hl = new QHBoxLayout();
  hl->addWidget(cancel);
  hl->addWidget(ok);

  l->addLayout(hl, Buttons, 1);

  setLayout(l);
}

void AmendDialog::authorDateTimeTypeChanged(const SelectedDateTimeType type) {
  const auto enabled = type == AmendDialog::SelectedDateTimeType::Manual;
  m_lAuthorCommitDate->setVisible(enabled);
  m_authorCommitDate->setVisible(enabled);
}

void AmendDialog::committerDateTimeTypeChanged(
    const SelectedDateTimeType type) {
  const auto enabled = type == AmendDialog::SelectedDateTimeType::Manual;
  m_lCommitterCommitDate->setVisible(enabled);
  m_committerCommitDate->setVisible(enabled);
}

QString AmendDialog::authorName() const { return m_authorName->text(); }

QString AmendDialog::authorEmail() const { return m_authorEmail->text(); }

QDateTime AmendDialog::authorCommitDate() const {
  if (authorCommitDateType() == SelectedDateTimeType::Original) {
    return m_author.date().toLocalTime();
  } else {
    return m_authorCommitDate->dateTime();
  }
}

AmendDialog::SelectedDateTimeType AmendDialog::authorCommitDateType() const {
  return m_authorCommitDateType->type();
}

QString AmendDialog::committerName() const { return m_committerName->text(); }

QString AmendDialog::committerEmail() const { return m_committerEmail->text(); }

QDateTime AmendDialog::committerCommitDate() const {
  if (committerCommitDateType() == SelectedDateTimeType::Original) {
    return m_committer.date().toLocalTime();
  } else {
    return m_committerCommitDate->dateTime();
  }
}

AmendDialog::SelectedDateTimeType AmendDialog::committerCommitDateType() const {
  return m_committerCommitDateType->type();
}

QString AmendDialog::commitMessage() const {
  return m_commitMessage->toPlainText();
}

#include "AmendDialog.moc"
