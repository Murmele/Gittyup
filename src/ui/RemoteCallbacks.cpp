//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "qtsupport.h"
#include "RemoteCallbacks.h"
#include "conf/Settings.h"
#include "cred/CredentialHelper.h"
#include "git/Command.h"
#include "git/Id.h"
#include "git/RevWalk.h"
#include "git2/sys/errors.h"
#include "log/LogEntry.h"
#include <libssh2.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFormLayout>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>
#include <QUrl>
#include <QUrlQuery>

namespace {

const int kKb = 1 << 10;
const int kMb = 1 << 20;
const int kGb = 1 << 30;

const QString kSizeFmt = "%1 %2";
const QString kRangeFmt = "%1..%2";
const QString kFromToFmt = "%1 -> %2";
const QString kUpdateFmt = " %1 %2 %3";
const QString kLinkFmt = "<a href='%1'>%2</a>";

QString size(int bytes) {
  if (bytes < kKb)
    return kSizeFmt.arg(bytes).arg("bytes");

  int i = 0;
  QString s;
  if (bytes < kMb) {
    i = kKb;
    s = "KiB";
  } else if (bytes < kGb) {
    i = kMb;
    s = "MiB";
  } else {
    i = kGb;
    s = "GiB";
  }

  return kSizeFmt.arg(static_cast<float>(bytes) / i, 0, 'f', 2).arg(s);
}

QString range(const QString &name, const QString &a, const QString &b) {
  QUrl url;
  url.setScheme("id");
  url.setPath(kRangeFmt.arg(a, b));

  QUrlQuery query;
  query.addQueryItem("ref", name);
  url.setQuery(query);

  return kLinkFmt.arg(url.toString(), kRangeFmt.arg(a.left(7), b.left(7)));
}

void setText(const QString &text, LogEntry *log, LogEntry *&entry) {
  if (entry) {
    entry->setText(text);
    return;
  }

  entry = log->addEntry(text);
}

} // namespace

RemoteCallbacks::RemoteCallbacks(Kind kind, LogEntry *log, const QString &url,
                                 const QString &name, QObject *parent,
                                 const git::Repository &repo)
    : QObject(parent), git::Remote::Callbacks(url, repo), mKind(kind),
      mLog(log), mName(name) {
  // Credentials and interactive have to block.
  QObject::connect(this, &RemoteCallbacks::queueCredentials, this,
                   &RemoteCallbacks::credentialsImpl,
                   Qt::BlockingQueuedConnection);
  QObject::connect(this, &RemoteCallbacks::queueInteractiveAuth, this,
                   &RemoteCallbacks::interactiveAuthImpl,
                   Qt::BlockingQueuedConnection);

  // The rest are automatic.
  QObject::connect(this, &RemoteCallbacks::queueSideband, this,
                   &RemoteCallbacks::sidebandImpl);
  QObject::connect(this, &RemoteCallbacks::queueTransfer, this,
                   &RemoteCallbacks::transferImpl);
  QObject::connect(this, &RemoteCallbacks::queueResolve, this,
                   &RemoteCallbacks::resolveImpl);
  QObject::connect(this, &RemoteCallbacks::queueUpdate, this,
                   &RemoteCallbacks::updateImpl);
  QObject::connect(this, &RemoteCallbacks::queueRejected, this,
                   &RemoteCallbacks::rejectedImpl);
  QObject::connect(this, &RemoteCallbacks::queueAdd, this,
                   &RemoteCallbacks::addImpl);
  QObject::connect(this, &RemoteCallbacks::queueDelta, this,
                   &RemoteCallbacks::deltaImpl);

  mTimer.start();
}

void RemoteCallbacks::setCanceled(bool canceled) {
  mCanceled = canceled;
  stop();
}

void RemoteCallbacks::storeDeferredCredentials() {
  // FIXME: Prompt user to remember?
  if (!mDeferredUrl.isEmpty()) {
    CredentialHelper *helper = CredentialHelper::instance();
    helper->store(mDeferredUrl, mDeferredUsername, mDeferredPassword);
  }
}

bool RemoteCallbacks::credentials(const QString &url, QString &username,
                                  QString &password) {
  if (mCanceled)
    return false;

  QString error;
  emit queueCredentials(url, username, password, error);

  // Set error on the thread that requested credentials.
  if (!error.isEmpty())
    git_error_set_str(GIT_ERROR_NET, error.toUtf8());

  return error.isEmpty();
}

void RemoteCallbacks::interactiveAuth(
    const QString &name, const QString &instruction,
    const QVector<git::Remote::SshInteractivePrompt> &prompts,
    QVector<QString> &responses) {
  if (mCanceled)
    return;

  QString error;
  emit queueInteractiveAuth(name, instruction, prompts, responses, error);

  // Set error on the thread that requested interactive auth.
  if (!error.isEmpty())
    git_error_set_str(GIT_ERROR_NET, error.toUtf8());
}

void RemoteCallbacks::sideband(const QString &text) {
  emit queueSideband(text, tr("remote: %1"));
}

bool RemoteCallbacks::transfer(int total, int current, size_t bytes) {
  int elapsed = mTimer.elapsed();
  if (current == 0 || current == total || elapsed > 100) {
    emit queueTransfer(total, current, bytes, elapsed);
    mTimer.restart();
  }

  return !mCanceled;
}

bool RemoteCallbacks::resolve(int total, int current) {
  if (current == 0 || current == total || mTimer.elapsed() > 100) {
    emit queueResolve(total, current);
    mTimer.restart();
  }

  return !mCanceled;
}

void RemoteCallbacks::update(const QString &name, const git::Id &a,
                             const git::Id &b) {
  emit queueUpdate(name, a, b);
}

void RemoteCallbacks::rejected(const QString &name, const QString &status) {
  emit queueRejected(name, status);
}

void RemoteCallbacks::add(int total, int current) {
  emit queueAdd(total, current);
}

void RemoteCallbacks::delta(int total, int current) {
  emit queueDelta(total, current);
}

bool RemoteCallbacks::negotiation(
    const QList<git::Remote::PushUpdate> &updates) {
  if (!mRepo.isValid())
    return true;

  QDir dir = mRepo.dir();
  if (!dir.exists("hooks/pre-push"))
    return true;

  QString bash = git::Command::bashPath();
  if (bash.isEmpty()) {
    QString error = tr("failed to execute pre-push hook: bash not found");
    git_error_set_str(GIT_ERROR_NET, error.toUtf8());
    return false;
  }

  QEventLoop loop;
  QProcess process;
  process.setWorkingDirectory(mRepo.workdir().path());
  using Signal = void (QProcess::*)(int, QProcess::ExitStatus);
  auto signal = static_cast<Signal>(&QProcess::finished);
  QObject::connect(&process, signal, &loop, &QEventLoop::exit);

  // Print hook output with the same semantics as sideband.
  QObject::connect(&process, &QProcess::readyReadStandardOutput,
                   [this, &process] {
                     emit queueSideband(process.readAllStandardOutput());
                     if (mCanceled)
                       process.terminate();
                   });

  // Start process.
  process.start(bash, {dir.filePath("hooks/pre-push"), mName, mUrl});

  // Write updates.
  QTextStream out(&process);
  foreach (const git::Remote::PushUpdate &update, updates)
    out << update.dstName << " " << update.dstId.toString() << " "
        << update.srcName << " " << update.srcId.toString() << Qt::endl;
  process.closeWriteChannel();

  if (loop.exec()) {
    QString error = process.readAllStandardError();
    if (error.isEmpty())
      error = process.errorString();

    QString text = tr("failed to execute pre-push hook: %1").arg(error);
    git_error_set_str(GIT_ERROR_NET, text.toUtf8());

    return false;
  }

  return true;
}

QString RemoteCallbacks::keyFilePath() const {
  return Settings::instance()->value(Setting::Id::SshKeyFilePath).toString();
}

QString RemoteCallbacks::configFilePath() const {
  return Settings::instance()->value(Setting::Id::SshConfigFilePath).toString();
}

bool RemoteCallbacks::connectToAgent() const {
  LIBSSH2_SESSION *session = libssh2_session_init();
  LIBSSH2_AGENT *agent = libssh2_agent_init(session);
  int error = libssh2_agent_connect(agent);
  if (error != LIBSSH2_ERROR_NONE) {
    char *msg;
    libssh2_session_last_error(session, &msg, nullptr, 0);
    git::Remote::log(QString("agent: %1 (%2)").arg(msg).arg(error));
  }

  libssh2_agent_disconnect(agent);
  libssh2_agent_free(agent);
  libssh2_session_free(session);
  return (error == LIBSSH2_ERROR_NONE);
}

void RemoteCallbacks::credentialsImpl(const QString &url, QString &username,
                                      QString &password, QString &error) {
  CredentialHelper *helper = CredentialHelper::instance();
  if (helper->get(url, username, password)) {
    QStringList key({url, username, password});
    if (!mQueriedCredentials.contains(key)) {
      mQueriedCredentials.insert(key);
      return;
    }
  }

  // Prompt for password.
  QDialog dialog;
  QString scheme = QUrl(url).scheme().toLower();
  bool https = (scheme == "http" || scheme == "https");
  dialog.setWindowTitle(https ? tr("HTTPS Credentials") : tr("SSH Passphrase"));

  QLineEdit *usernameField = https ? new QLineEdit(username, &dialog) : nullptr;
  QLineEdit *passwordField = new QLineEdit(&dialog);
  passwordField->setEchoMode(QLineEdit::Password);

  QDialogButtonBox *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog,
                   &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);

  QFormLayout *layout = new QFormLayout(&dialog);
  if (usernameField)
    layout->addRow(tr("Username:"), usernameField);
  layout->addRow(https ? tr("Password:") : tr("Passphrase:"), passwordField);
  layout->addRow(buttons);

  auto updateButtons = [usernameField, passwordField, buttons] {
    bool password = !passwordField->text().isEmpty();
    bool username = (!usernameField || !usernameField->text().isEmpty());
    buttons->button(QDialogButtonBox::Ok)->setEnabled(username && password);
  };

  updateButtons();
  if (usernameField)
    QObject::connect(usernameField, &QLineEdit::textChanged, updateButtons);
  QObject::connect(passwordField, &QLineEdit::textChanged, updateButtons);

  if (!dialog.exec()) {
    error = tr("authentication canceled");
    return;
  }

  if (usernameField)
    username = usernameField->text();
  password = passwordField->text();

  // Remember in keychain.
  mDeferredUrl = url;
  mDeferredUsername = username;
  mDeferredPassword = password;
}

void RemoteCallbacks::interactiveAuthImpl(
    const QString &name, const QString &instruction,
    const QVector<git::Remote::SshInteractivePrompt> &prompts,
    QVector<QString> &responses, QString &error) {
  QDialog dialog;
  dialog.setWindowTitle("SSH interactive authentication");

  QDialogButtonBox *buttons = new QDialogButtonBox(&dialog);
  buttons->addButton(QDialogButtonBox::Ok);
  buttons->addButton(QDialogButtonBox::Cancel);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog,
                   &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);

  QVector<QLineEdit *> inputs(prompts.length());
  QFormLayout *form = new QFormLayout(&dialog);

  for (int i = 0; i < prompts.length(); ++i) {
    QLineEdit *edit = new QLineEdit(&dialog);

    if (!prompts[i].echo)
      edit->setEchoMode(QLineEdit::EchoMode::Password);

    form->addRow(prompts[i].text, edit);
    inputs[i] = edit;
  }

  form->addRow(buttons);

  if (inputs.length() > 0)
    inputs[0]->setFocus();

  if (!dialog.exec()) {
    error = tr("authentication canceled");
    return;
  }

  for (int i = 0; i < prompts.length(); ++i)
    responses[i] = inputs[i]->text();
}

void RemoteCallbacks::sidebandImpl(const QString &text, const QString &fmt) {
  // Build up sideband text until carriage return or newline.
  // Carriage return overwrites existing entry. Newline emits new.
  mSideband.append(text);

  QRegularExpression re("\\r|\\n");
  int index = mSideband.indexOf(re);
  while (index >= 0) {
    QString line = mSideband.left(index);
    QString text = fmt.isEmpty() ? line : fmt.arg(line);
    if (mSidebandItem) {
      mSidebandItem->setText(text);
    } else {
      // Insert before the transfer item.
      const QList<LogEntry *> &entries = mLog->entries();
      mSidebandItem = !mTransferItem
                          ? mLog->addEntry(text)
                          : mLog->insertEntry(entries.indexOf(mTransferItem),
                                              LogEntry::Entry, text);
    }

    // Start new item.
    if (mSideband.at(index) == '\n')
      mSidebandItem = nullptr;

    mSideband.remove(0, index + 1);
    index = mSideband.indexOf(re);
  }
}

void RemoteCallbacks::transferImpl(int total, int current, int bytes,
                                   int elapsed) {
  // Change state to resolve.
  if (current == total)
    mState = Resolve;

  if (total == 0)
    return;

  // Calculate percent.
  int percent = 100 * (static_cast<float>(current) / total);

  // Calculate the transfer rate.
  float seconds = static_cast<float>(qMax(elapsed, 1)) / 1000;
  int transferRate = (bytes - mBytesReceived) / seconds;
  mBytesReceived = bytes;

  // Write text.
  QString text;
  QTextStream stream(&text);
  stream << ((mKind == Receive) ? "Receiving" : "Writing")
         << " objects: " << percent << "% (" << current << "/" << total << "), "
         << size(bytes) << " | " << size(transferRate) << "/s"
         << ((mState != Transfer) ? ", done" : QString()) << ".";

  // Update the list item.
  setText(text, mLog, mTransferItem);
}

void RemoteCallbacks::resolveImpl(int total, int current) {
  // Calculate percent.
  int percent = (total > 0) ? 100 * (static_cast<float>(current) / total) : 0;

  // Write text.
  QString text;
  QTextStream stream(&text);
  stream << "Resolving deltas: " << percent << "% (" << current << "/" << total
         << ")" << (current == total ? ", done" : QString()) << ".";

  // Update the list item.
  setText(text, mLog, mResolveItem);
}

void RemoteCallbacks::updateImpl(const QString &name, const git::Id &a,
                                 const git::Id &b) {
  if (mState == Resolve) {
    // Write header.
    QString fmt = (mKind == Receive) ? tr("From %1") : tr("To %1");
    mLog->addEntry(fmt.arg(mUrl));
    mState = Update;
  }

  // Signal attached views.
  emit referenceUpdated(name);

  QChar flag(' ');
  QString astr = a.toString();
  QString bstr = b.toString();
  QString summary = range(name, astr, bstr);
  QString remoteName = name.section('/', 2);
  QString localName = remoteName.section('/', 1);
  QString fromTo = kFromToFmt.arg(localName, remoteName);

  QRegularExpression re("[^0]");
  if (!astr.contains(re)) {
    flag = '*';
    bool tag = (name.section('/', 1, 1) == "tags");
    summary = QString("[new %1]").arg(tag ? "tag" : "branch");
  } else if (!bstr.contains(re)) {
    flag = '-';
    summary = "[deleted]";
    fromTo = localName;
  } else if (mRepo.isValid()) {
    git::Commit lhs = mRepo.lookupCommit(a);
    git::Commit rhs = mRepo.lookupCommit(b);
    if (lhs.isValid() && rhs.isValid()) {
      git::Commit base = mRepo.mergeBase(lhs, rhs);
      if (!base.isValid() || base != lhs)
        fromTo.append(" (forced update)");
    }
  }

  // Write text.
  mLog->addEntry(kUpdateFmt.arg(flag, summary, fromTo));
}

void RemoteCallbacks::rejectedImpl(const QString &name, const QString &status) {
  if (mState == Resolve) {
    // Write header.
    mLog->addEntry(tr("To %1").arg(mUrl));
    mState = Update;
  }

  QString relativeName = name.section('/', 2);
  QString fromTo = kFromToFmt.arg(relativeName, relativeName);
  QString text = QString("[remote rejected] %1 (%2)").arg(fromTo, status);
  mLog->addEntry(LogEntry::Error, text);
}

void RemoteCallbacks::addImpl(int total, int current) {
  // Write text.
  QString text;
  QTextStream stream(&text);
  stream << "Counting objects: " << current
         << (current == total ? ", done" : QString()) << ".";

  // Update the list item.
  setText(text, mLog, mAddItem);
}

void RemoteCallbacks::deltaImpl(int total, int current) {
  // Finish off the add stage.
  if (!mDeltaItem)
    addImpl(total, total);

  // Write text.
  QString text;
  QTextStream stream(&text);
  stream << "Compressing objects: "
         << static_cast<int>(100 * (static_cast<float>(current) / total))
         << "% (" << current << "/" << total << ")"
         << (current == total ? ", done" : QString()) << ".";

  // Update the list item.
  setText(text, mLog, mDeltaItem);

  // Delta compression only reports progress once at the beginning.
  // Post another update for completion after processing events.
  if (current != total)
    deltaImpl(total, total);
}
