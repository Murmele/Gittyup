//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Kas
//

#include "DeleteTagDialog.h"
#include "git/TagRef.h"
#include "git/Config.h"
#include "git/Remote.h"
#include "log/LogEntry.h"
#include "ui/RemoteCallbacks.h"
#include "ui/RepoView.h"
#include <QCheckBox>
#include <QFutureWatcher>
#include <QPushButton>
#include <QtConcurrent>

DeleteTagDialog::DeleteTagDialog(const git::TagRef &tag, QWidget *parent)
    : QMessageBox(parent) {
  QString text = tr("Are you sure you want to delete tag '%1'?");
  setWindowTitle(tr("Delete Tag?"));
  setText(text.arg(tag.name()));
  setStandardButtons(QMessageBox::Cancel);

  git::Remote remote = tag.repo().defaultRemote();

  if (remote.isValid()) {
    text = tr("Also delete the upstream tag from %1");
    setCheckBox(new QCheckBox(text.arg(remote.name()), this));
  }

  QPushButton *remove = addButton(tr("Delete"), QMessageBox::AcceptRole);
  connect(remove, &QPushButton::clicked, [this, tag, remote] {
    RepoView *view = RepoView::parentView(this);
    QString name = tag.name();

    if (remote.isValid() && checkBox()->isChecked()) {
      git::Repository repo = view->repo();

      QString remoteName = remote.name();
      QString text = tr("delete '%1' from '%2'").arg(name, remoteName);
      LogEntry *entry = view->addLogEntry(text, tr("Push"));

      QFutureWatcher<git::Result> *watcher =
          new QFutureWatcher<git::Result>(view);
      RemoteCallbacks *callbacks =
          new RemoteCallbacks(RemoteCallbacks::Send, entry, remote.url(),
                              remoteName, watcher, repo);

      entry->setBusy(true);
      QStringList refspecs(QString(":refs/tags/%1").arg(name));
      git::Result (git::Remote::*push)(git::Remote::Callbacks*, const QStringList&) = &git::Remote::push;
      watcher->setFuture(
          QtConcurrent::run(push, remote, callbacks, refspecs));

      connect(watcher, &QFutureWatcher<git::Result>::finished, watcher,
              [entry, watcher, callbacks, remoteName] {
                entry->setBusy(false);
                git::Result result = watcher->result();
                if (callbacks->isCanceled()) {
                  entry->addEntry(LogEntry::Error, tr("Push canceled."));
                } else if (!result) {
                  QString err = result.errorString();
                  QString fmt = tr("Unable to push to %1 - %2");
                  entry->addEntry(LogEntry::Error, fmt.arg(remoteName, err));
                }

                watcher->deleteLater();
              });
    }

    if (!git::TagRef(tag).remove()) {
      LogEntry *parent = view->addLogEntry(name, tr("Delete Tag"));
      view->error(parent, tr("delete tag"), name);
      return;
    }
  });

  remove->setFocus();
}
