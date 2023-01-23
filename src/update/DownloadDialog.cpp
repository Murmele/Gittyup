//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "DownloadDialog.h"
#include "dialogs/IconLabel.h"
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QIcon>
#include <QLabel>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

DownloadDialog::DownloadDialog(const Updater::DownloadRef &download,
                               QWidget *parent)
    : QDialog(parent) {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Update %1").arg(QCoreApplication::applicationName()));

  QIcon icon(":/Gittyup.iconset/icon_128x128.png");
  IconLabel *iconLabel = new IconLabel(icon, 64, 64, this);

  QVBoxLayout *iconLayout = new QVBoxLayout;
  iconLayout->addWidget(iconLabel);
  iconLayout->addStretch();

  QLabel *label = new QLabel(tr("Downloading %1...").arg(download->name()));
  QProgressBar *progress = new QProgressBar(this);

  QDialogButtonBox *buttons =
      new QDialogButtonBox(QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::rejected, this, &DownloadDialog::reject);
  connect(buttons, &QDialogButtonBox::accepted, this, &DownloadDialog::accept);

  QVBoxLayout *content = new QVBoxLayout;
  content->addWidget(label);
  content->addWidget(progress);
  content->addWidget(buttons);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addLayout(iconLayout);
  layout->addLayout(content);

  QNetworkReply *reply = download->reply();
  connect(reply, &QNetworkReply::finished, [this, label, buttons, reply] {
    // The download was aborted.
    if (reply->error() != QNetworkReply::NoError || !reply->isOpen()) {
      close();
      return;
    }

    // Adjust interface for installation.
    label->setText(tr("Download Complete!"));
    QPushButton *install = buttons->addButton(tr("Install and Restart"),
                                              QDialogButtonBox::AcceptRole);
    install->setDefault(true);

    // Now connect reject to the cancel signal.
    connect(this, &DownloadDialog::rejected, Updater::instance(),
            &Updater::updateCanceled);
  });

  // Handle reject and accept.
  connect(this, &DownloadDialog::rejected, reply, &QNetworkReply::abort);
  connect(this, &DownloadDialog::accepted,
          [download] { Updater::instance()->install(download); });

  // Show download progress.
  connect(reply, &QNetworkReply::downloadProgress,
          [progress](int current, int total) {
            progress->setMaximum(total);
            progress->setValue(current);
          });
}
