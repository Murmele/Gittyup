//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "AccountDialog.h"
#include "cred/CredentialHelper.h"
#include "host/Accounts.h"
#include "ui/ExpandButton.h"
#include <QApplication>
#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>

AccountDialog::AccountDialog(Account *account, QWidget *parent)
    : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Add Remote Account"));

  mHost = new QComboBox(this);
  mHost->setMinimumWidth(mHost->sizeHint().width() * 2);
  mHost->addItem("GitHub", Account::GitHub);
  mHost->addItem("Gitea", Account::Gitea);
  mHost->addItem("Bitbucket", Account::Bitbucket);
  mHost->addItem("Beanstalk", Account::Beanstalk);
  mHost->addItem("GitLab", Account::GitLab);

  Account::Kind kind = account ? account->kind() : Account::GitHub;
  setKind(kind);

  mUsername = new QLineEdit(this);
  mUsername->setText(account ? account->username() : QString());
  connect(mUsername, &QLineEdit::textChanged, this,
          &AccountDialog::updateButtons);

  mPassword = new QLineEdit(this);
  mPassword->setEchoMode(QLineEdit::Password);
  mPassword->setText(account ? account->password() : QString());
  connect(mPassword, &QLineEdit::textChanged, this,
          &AccountDialog::updateButtons);

  auto signal = QOverload<int>::of(&QComboBox::currentIndexChanged);
  mLabel = new QLabel(Account::helpText(kind), this);
  mLabel->setWordWrap(true);
  mLabel->setOpenExternalLinks(true);
  mLabel->setVisible(!mLabel->text().isEmpty());
  connect(mHost, signal, [this] {
    Account::Kind kind =
        static_cast<Account::Kind>(mHost->currentData().toInt());
    mLabel->setText(Account::helpText(kind));
    mLabel->setVisible(!mLabel->text().isEmpty());
  });

  ExpandButton *expand = new ExpandButton(this);
  QWidget *advanced = new QWidget(this);
  advanced->setVisible(false);

  QFormLayout *form = new QFormLayout;
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->addRow(tr("Host:"), mHost);
  form->addRow(tr("Username:"), mUsername);
  form->addRow(tr("Password:"), mPassword);
  form->addRow(mLabel);
  form->addRow(tr("Advanced:"), expand);

  mUrl = new QLineEdit(advanced);
  mUrl->setText(account ? account->url() : Account::defaultUrl(kind));
  connect(mHost, signal, [this] {
    Account::Kind kind =
        static_cast<Account::Kind>(mHost->currentData().toInt());
    mUrl->setText(Account::defaultUrl(kind));
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    resize(sizeHint());
  });

  QFormLayout *advancedForm = new QFormLayout(advanced);
  advancedForm->setContentsMargins(-1, 0, 0, 0);
  advancedForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  advancedForm->addRow(tr("URL:"), mUrl);

  connect(expand, &ExpandButton::toggled, [this, advanced](bool checked) {
    advanced->setVisible(checked);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    resize(sizeHint());
  });

  QDialogButtonBox::StandardButtons buttons =
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
  mButtons = new QDialogButtonBox(buttons, this);
  mButtons->button(QDialogButtonBox::Ok)->setEnabled(false);
  connect(mButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(mButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(form);
  layout->addWidget(advanced);
  layout->addWidget(mButtons);

  updateButtons();
}

void AccountDialog::accept() {
  // Validate account.
  Account::Kind kind = static_cast<Account::Kind>(mHost->currentData().toInt());
  QString username = mUsername->text();
  QString url =
      (mUrl->text() != Account::defaultUrl(kind)) ? mUrl->text() : QString();

  if (Account *account = Accounts::instance()->lookup(username, kind)) {
    QMessageBox mb(QMessageBox::Information, tr("Replace?"),
                   tr("An account of this type already exists."));
    mb.setInformativeText(
        tr("Would you like to replace the previous account?"));
    QPushButton *remove = mb.addButton(tr("Replace"), QMessageBox::AcceptRole);
    mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
    mb.setDefaultButton(remove);
    mb.exec();

    if (mb.clickedButton() != remove)
      return;

    Accounts::instance()->removeAccount(account);
  }

  Account *account = Accounts::instance()->createAccount(kind, username, url);
  AccountProgress *progress = account->progress();
  connect(progress, &AccountProgress::finished, this, [this, account] {
    AccountError *error = account->error();
    if (error->isValid()) {
      QString text = error->text();
      QString title = tr("Connection Failed");
      QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok);
      msg.setInformativeText(error->detailedText());
      msg.exec();

      Accounts::instance()->removeAccount(account);
      return;
    }

    // Store password.
    QUrl url;
    url.setScheme("https");
    url.setHost(account->host());

    CredentialHelper *helper = CredentialHelper::instance();
    helper->store(url.toString(), account->username(), mPassword->text());

    QDialog::accept();
  });

  // Start asynchronous connection.
  account->connect(mPassword->text());
}

void AccountDialog::setKind(Account::Kind kind) {
  mHost->setCurrentIndex(mHost->findData(kind));
}

void AccountDialog::updateButtons() {
  mButtons->button(QDialogButtonBox::Ok)
      ->setEnabled(!mUsername->text().isEmpty() &&
                   !mPassword->text().isEmpty());
}
