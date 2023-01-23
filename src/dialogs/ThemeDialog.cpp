//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "ThemeDialog.h"
#include "conf/Settings.h"
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

class ThemeButton : public QPushButton {
  Q_OBJECT

public:
  enum class Theme { Default, Dark, System };

  ThemeButton(const QString &title, const QIcon &icon,
              const QString &description, const Theme &theme,
              QWidget *parent = nullptr)
      : QPushButton(parent), mTheme(theme) {
    setStyleSheet("ThemeButton #description {"
                  "  font-size: 12px;"
                  "  font-style: italics;"
                  "  margin: 0px 0px 16px 0px"
                  "}"

                  "ThemeButton #title {"
                  "  font-weight: bold;"
                  "  font-size: 16px;"
                  "  margin: 13px 0px 20px 8px"
                  "}");

    QSize iconSize = QSize(245, 196);
    setFixedHeight(iconSize.height() + 80);
    setFixedWidth(iconSize.width() + 35);
    setFocusPolicy(Qt::StrongFocus);

    mTitle = new QLabel(title, this);
    mTitle->setObjectName("title");

    setIcon(icon);
    setIconSize(iconSize);

    mDescription = new QLabel(description, this);
    mDescription->setAlignment(Qt::AlignHCenter);
    mDescription->setMinimumWidth(rect().width());
    mDescription->setObjectName("description");

    connect(this, &QPushButton::clicked, [this] {
      switch (mTheme) {
        case Theme::System:
          Settings::instance()->setValue(Setting::Id::ColorTheme, "System");
          break;
        case Theme::Dark:
          Settings::instance()->setValue(Setting::Id::ColorTheme, "Dark");
          break;
        default:
          Settings::instance()->setValue(Setting::Id::ColorTheme, "Default");
          break;
      }

      window()->close();
    });
  }

  void resizeEvent(QResizeEvent *event) override {
    QSize size = mDescription->sizeHint();
    mDescription->move(rect().left(), height() - (size.height() + 5));
    mTitle->move(rect().left() + 5, rect().top());
  }

private:
  QLabel *mTitle;
  QLabel *mDescription;
  Theme mTheme;
};

} // namespace

ThemeDialog::ThemeDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle(tr("Pick a theme for Gittyup"));

  ThemeButton *native = new ThemeButton(
      tr("Default Theme"), QIcon(":/native.png"),
      tr("A consistent bright theme"), ThemeButton::Theme::Default);

  ThemeButton *dark =
      new ThemeButton(tr("Dark Theme"), QIcon(":/dark.png"),
                      tr("A consistent look optimal for reducing eye strain"),
                      ThemeButton::Theme::Dark);

  ThemeButton *system = new ThemeButton(
      tr("System Theme"), QIcon(":/system.png"),
      tr("A flexible look matching system colors"), ThemeButton::Theme::System);

  QHBoxLayout *themeButtons = new QHBoxLayout;
  themeButtons->addWidget(native);
  themeButtons->addWidget(dark);
  themeButtons->addSpacing(20);
  themeButtons->addWidget(system);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(themeButtons);
}

#include "ThemeDialog.moc"
