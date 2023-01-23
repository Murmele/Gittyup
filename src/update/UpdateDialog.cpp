//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "UpdateDialog.h"
#include "DownloadDialog.h"
#include "Updater.h"
#include "ui/MenuBar.h"
#include "conf/Settings.h"
#include "dialogs/IconLabel.h"
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QMessageBox>

namespace {

const QString kStyleSheet = "h3 {"
                            "  color: #696969;"
                            "  text-decoration: underline"
                            "}";

} // namespace

UpdateDialog::UpdateDialog(const QString &platform, const QString &version,
                           const QString &changelog, const QString &link,
                           QWidget *parent)
    : QDialog(parent) {
  QString appName = QCoreApplication::applicationName();
  QString appVersion = QCoreApplication::applicationVersion();

  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Update %1").arg(appName));

  QIcon icon(":/Gittyup.iconset/icon_128x128.png");
  IconLabel *iconLabel = new IconLabel(icon, 128, 128, this);

  QVBoxLayout *iconLayout = new QVBoxLayout;
  iconLayout->addWidget(iconLabel);
  iconLayout->addStretch();

#ifndef Q_OS_LINUX
  QString label = tr("<h3>A new version of %1 is available!</h3>"
                     "<p>%1 %2 is now available - you have %3. "
                     "Would you like to download it now?</p>"
                     "<b>Release Notes:</b>")
                      .arg(appName, version, appVersion);
#elif defined(FLATPAK) || defined(DEBUG_FLATPAK)
  QString label =
      tr("<h3>A new version of %1 is available!</h3>"
         "<p>%1 %2 is now available - you have %3.</p>"
         "<p>If you downloaded the flatpak package over a package manager or "
         "from flathub.org <br/>"
         "you don't have to install manually a new version. It will be "
         "available within the next <br/>"
         "days during your system update: <code>flatpak update</code></p>"
         "<b>Release Notes:</b>")
          .arg(appName, version, appVersion);
#else
  QString label = tr("<h3>A new version of %1 is available!</h3>"
                     "<p>%1 %2 is now available - you have %3. "
                     "The new version will be soon available in your package "
                     "manager. Just update your system.</p>"
                     "<b>Release Notes:</b>")
                      .arg(appName, version, appVersion);
#endif

  QTextBrowser *browser = new QTextBrowser;
  browser->document()->setDocumentMargin(12);
  browser->document()->setDefaultStyleSheet(kStyleSheet);
  browser->setHtml(changelog);

#if !defined(Q_OS_LINUX) || defined(FLATPAK) || defined(DEBUG_FLATPAK)
  QCheckBox *download =
      new QCheckBox(tr("Automatically download and install updates"), this);
  download->setChecked(Settings::instance()
                           ->value(Setting::Id::InstallUpdatesAutomatically)
                           .toBool());
  connect(download, &QCheckBox::toggled, [](bool checked) {
    Settings::instance()->setValue(Setting::Id::InstallUpdatesAutomatically,
                                   checked);
  });
#endif

  QDialogButtonBox *buttons = new QDialogButtonBox(this);
#if !defined(Q_OS_LINUX) || defined(FLATPAK) || defined(DEBUG_FLATPAK)
  buttons->addButton(tr("Install Update"), QDialogButtonBox::AcceptRole);
#endif

  buttons->addButton(tr("Remind Me Later"), QDialogButtonBox::RejectRole);
  connect(buttons, &QDialogButtonBox::accepted, this, &UpdateDialog::accept);

  QPushButton *skip =
      buttons->addButton(tr("Skip This Version"), QDialogButtonBox::ResetRole);

  connect(skip, &QPushButton::clicked, [this, version] {
    Settings *settings = Settings::instance();
    QStringList skipped =
        settings->value(Setting::Id::SkippedUpdates).toStringList();
    if (!skipped.contains(version))
      settings->setValue(Setting::Id::SkippedUpdates, skipped << version);
    reject();
  });

  connect(buttons, &QDialogButtonBox::rejected, this, &UpdateDialog::reject);

  QHBoxLayout *l = new QHBoxLayout();
  QPushButton *supportButton =
      new QPushButton(QIcon(":/liberapay_icon_130890.png"), tr("Donate"), this);
  QSpacerItem *spacer =
      new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
#if !defined(Q_OS_LINUX) || defined(FLATPAK) || defined(DEBUG_FLATPAK)
  l->addWidget(download);
#endif
  l->addItem(spacer);
  l->addWidget(supportButton);
  connect(supportButton, &QPushButton::pressed, []() {
    QDesktopServices::openUrl(QUrl(MenuBar::donationUrlLiberapay));
  });

  QVBoxLayout *content = new QVBoxLayout;
  content->addWidget(new QLabel(label, this));
  content->addWidget(browser);
  content->addLayout(l);
  content->addWidget(buttons);

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->addLayout(iconLayout);
  layout->addLayout(content);

  connect(this, &UpdateDialog::accepted, [platform, link] {
    // Start download.
    if (Updater::DownloadRef download = Updater::instance()->download(link)) {
      DownloadDialog *dialog = new DownloadDialog(download);
      dialog->show();
    }
  });
}
