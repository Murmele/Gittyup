//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "Theme.h"
#include "CustomTheme.h"
#include "app/Application.h"
#include "conf/ConfFile.h"
#include "conf/Settings.h"
#include "dialogs/ThemeDialog.h"
#include "ui/DiffView/DiffView.h"
#include <QProxyStyle>
#include <QStyleOption>
#include <QWidget>

namespace {

class Style : public QProxyStyle {
public:
  Style(const Theme *theme) : mTheme(theme) {}

  void polish(QPalette &palette) override {
    baseStyle()->polish(palette);
    mTheme->polish(palette);
  }

private:
  const Theme *mTheme;
};

} // namespace

Theme::Theme() {
  mDir = Settings::themesDir();
  mName = QString("System");

  // Create Qt theme.
  QFile themeFile(mDir.filePath(QString("%1.lua").arg(mName)).toUtf8());
  if (themeFile.open(QIODevice::ReadOnly)) {
    QDir tempDir = QDir::temp();
    QFile tempFile(tempDir.filePath(QString("%1.lua").arg(mName)).toUtf8());
    if (tempFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
      mDir = tempDir;

      // Copy template.
      tempFile.write(themeFile.readAll());

      // Add theme colors for scintilla editor.
      tempFile.write(
          QString("theme.property['style.default']      = 'fore:%1,back:%2'\n")
              .arg(QPalette().color(QPalette::Text).name(QColor::HexRgb),
                   QPalette().color(QPalette::Base).name(QColor::HexRgb))
              .toUtf8());
      tempFile.close();
    }
    themeFile.close();
  }

  // Load Qt theme.
  QByteArray file = mDir.filePath(QString("%1.lua").arg(mName)).toUtf8();
  mMap = ConfFile(file).parse("theme");

  QPalette palette;
  QColor base = palette.color(QPalette::Base);
  QColor text = palette.color(QPalette::Text);
  mDark = (text.lightnessF() > base.lightnessF());
}

QString Theme::diffButtonStyle(Theme::Diff role) {
  QColor color = diff(role);
  QString pressed = color.darker(115).name();
  return DiffViewStyle::kButtonStyleFmt.arg(color.name(), pressed);
}

QDir Theme::dir() const { return mDir; }

QString Theme::name() const { return mName; }

QStyle *Theme::style() const { return new Style(this); }

QString Theme::styleSheet() const { return QString(); }

void Theme::polish(QPalette &palette) const {
  palette.setColor(QPalette::BrightText, palette.color(QPalette::Text));
  palette.setColor(QPalette::Light, palette.color(QPalette::Dark));
  palette.setColor(QPalette::Shadow, palette.color(QPalette::Mid));
}

QColor Theme::badge(BadgeRole role, BadgeState state) {
  switch (role) {
    case BadgeRole::Foreground:
      switch (state) {
        case BadgeState::Selected:
          return QPalette().color(QPalette::Text);

        case BadgeState::Head:
          return QPalette().color(QPalette::HighlightedText);

        default:
          return QPalette().color(QPalette::WindowText);
      }

    case BadgeRole::Background:
      switch (state) {
        case BadgeState::Normal:
          return mDark
                     ? QPalette().color(QPalette::Inactive, QPalette::Highlight)
                     : QPalette().color(QPalette::Mid);

        case BadgeState::Selected:
          return QPalette().color(QPalette::Base);

        case BadgeState::Conflicted:
          return mDark ? "#DA2ADA" : "#D22222";

        case BadgeState::Head:
          return QPalette().color(QPalette::Highlight);

        case BadgeState::Notification:
          return mDark ? "#8C2026" : "#FF0000";
        case BadgeState::Modified:
          return mDark ? "#91973A" : "#FFEEDB";
        case BadgeState::Added:
          return mDark ? "#394734" : "#DCFFDC";
        case BadgeState::Deleted:
          return mDark ? "#5E3638" : "#FFDCDC";
        case BadgeState::Untracked:
          return mDark ? "#2A4944" : "#C7FFF6";
        case BadgeState::Renamed:
          return mDark ? "#23455E" : "#D2E8FC";
      }
  }
  assert(false);
  return QStringLiteral("");
}

QList<QColor> Theme::branchTopologyEdges() {
  QVariantMap edge = mMap.value("graph").toMap();

  QList<QColor> colors;
  for (int i = 0; i < 15; i++) {
    QString name = QString("edge%1").arg(i + 1);
    colors.append(edge.value(name).toString());
  }

  return colors;
}

QColor Theme::buttonChecked() { return QPalette().color(QPalette::Highlight); }

QPalette Theme::commitList() { return QPalette(); }

QColor Theme::commitEditor(CommitEditor color) {
  switch (color) {
    case CommitEditor::SpellError:
      return Qt::red;
    case CommitEditor::SpellIgnore:
      return Qt::gray;
    case CommitEditor::LengthWarning:
      return Qt::yellow;
  }
}

QColor Theme::diff(Diff color) {
  if (mDark) {
    switch (color) {
      case Diff::Ours:
        return "#000060";
      case Diff::Theirs:
        return "#600060";
      case Diff::Addition:
        return "#394734";
      case Diff::Deletion:
        return "#5E3638";
      case Diff::WordAddition:
        return "#296812";
      case Diff::WordDeletion:
        return "#781B20";
      case Diff::Plus:
        return "#207A00";
      case Diff::Minus:
        return "#BC0009";
      case Diff::Note:
        return "#E1E5F2";
      case Diff::Warning:
        return "#E8C080";
      case Diff::Error:
        return "#7E494B";
    }
  }

  switch (color) {
    case Diff::Ours:
      return "#DCFFFF";
    case Diff::Theirs:
      return "#FFDCFF";
    case Diff::Addition:
      return "#DCFFDC";
    case Diff::Deletion:
      return "#FFDCDC";
    case Diff::WordAddition:
      return "#B0F2B0";
    case Diff::WordDeletion:
      return "#F2B0B0";
    case Diff::Plus:
      return "#45CC45";
    case Diff::Minus:
      return "#F28080";
    case Diff::Note:
      return "#000000";
    case Diff::Warning:
      return "#FFFF00";
    case Diff::Error:
      return "#FF0000";
  }
}

QColor Theme::heatMap(HeatMap color) {
  switch (color) {
    case HeatMap::Hot:
      return QPalette().color(QPalette::Highlight);
    case HeatMap::Cold:
      return mDark ? QPalette().color(QPalette::Inactive, QPalette::Highlight)
                   : QPalette().color(QPalette::Mid);
  }
}

QColor Theme::remoteComment(Comment color) {
  switch (color) {
    case Comment::Background:
      return QPalette().color(QPalette::Base);
    case Comment::Body:
      return QPalette().color(QPalette::Window);
    case Comment::Author:
      return QPalette().color(QPalette::WindowText);
    case Comment::Timestamp:
      return QPalette().color(QPalette::WindowText);
  }
}

QColor Theme::star() { return QPalette().color(QPalette::Highlight); }

Theme *Theme::create(const QString &defaultName) {
  // Upgrade theme key to capital case.
  Settings *settings = Settings::instance();
  QString key = settings->value(Setting::Id::ColorTheme).toString();
  if (key == "default" || key == "dark" || key == "system") {
    key[0] = key.at(0).toUpper();
    settings->setValue(Setting::Id::ColorTheme, key);
  }

  QString name = !defaultName.isEmpty() ? defaultName : key;
  if (name.isEmpty() && !Application::isInTest()) {
    ThemeDialog dialog;
    dialog.exec();
    name = settings->value(Setting::Id::ColorTheme).toString();
  }

  // Load custom theme.
  if (CustomTheme::isValid(name) && !name.contains("System"))
    return new CustomTheme(name);

  // Use Qt theme.
  return new Theme();
}
