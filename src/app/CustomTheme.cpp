//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Shane Gramlich
//

#include "CustomTheme.h"
#include "conf/ConfFile.h"
#include "conf/Settings.h"
#include <QDir>
#include <QPainter>
#include <QProxyStyle>
#include <QStandardPaths>
#include <QStyleOptionButton>

namespace {

void setPaletteColors(QPalette &palette, QPalette::ColorRole role,
                      const QVariant &variant) {
  if (variant.canConvert<QColor>()) {
    palette.setColor(role, variant.value<QColor>());
    return;
  }

  QVariantMap map = variant.toMap();
  foreach (const QString &key, map.keys()) {
    QColor color(map.value(key).toString());
    if (!color.isValid()) {
      Q_ASSERT(false);
      continue;
    }

    if (key == "default") {
      palette.setColor(role, color);
    } else if (key == "active") {
      palette.setColor(QPalette::Active, role, color);
    } else if (key == "inactive") {
      palette.setColor(QPalette::Inactive, role, color);
    } else if (key == "disabled") {
      palette.setColor(QPalette::Disabled, role, color);
    }
  }
}

class CustomStyle : public QProxyStyle {
public:
  CustomStyle(const CustomTheme *theme)
      : QProxyStyle("fusion"), mTheme(theme) {}

  void drawPrimitive(PrimitiveElement elem, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override {
    switch (elem) {
      case PE_IndicatorCheckBox: {
        QStyleOptionButton button;
        button.QStyleOption::operator=(*option);

        QVariantMap checkbox = mTheme->checkbox();
        button.palette.setColor(QPalette::Text, Qt::white);
        setPaletteColors(button.palette, QPalette::Base,
                         checkbox.value("fill"));
        setPaletteColors(button.palette, QPalette::Window,
                         checkbox.value("outline"));
        setPaletteColors(button.palette, QPalette::Text,
                         checkbox.value("text"));
        baseStyle()->drawPrimitive(elem, &button, painter, widget);
        break;
      }

      case PE_IndicatorTabClose:
        CustomTheme::drawCloseButton(option, painter);
        break;

      default:
        baseStyle()->drawPrimitive(elem, option, painter, widget);
        break;
    }
  }

  void polish(QWidget *widget) override {
    baseStyle()->polish(widget);
    if (QWindow *window = widget->windowHandle())
      mTheme->polishWindow(window);
  }

  void polish(QPalette &palette) override {
    baseStyle()->polish(palette);
    mTheme->polish(palette);
  }

private:
  const CustomTheme *mTheme;
};

} // namespace

CustomTheme::CustomTheme(const QString &name) : mName(name) {
  mDir = Settings::themesDir();
  if (!QFileInfo(mDir.filePath(QString("%1.lua").arg(name))).exists())
    mDir = userDir();

  QByteArray file = mDir.filePath(QString("%1.lua").arg(name)).toUtf8();
  mMap = ConfFile(file).parse("theme");
}

QDir CustomTheme::dir() const { return mDir; }

QString CustomTheme::name() const { return mName; }

QStyle *CustomTheme::style() const { return new CustomStyle(this); }

QString CustomTheme::styleSheet() const {
  QString text = "QLineEdit {"
                 "  border: 1px solid palette(shadow)"
                 "}"

                 "QToolButton:hover {"
                 "  background: palette(dark)"
                 "}"
                 "QToolButton:pressed {"
                 "  background: %1"
                 "}"

                 "DetailView QTextEdit {"
                 "  border: 1px solid palette(shadow)"
                 "}"
                 "DetailView QTextEdit#MessageLabel {"
                 "  background: palette(window);"
                 "  border: 1px solid palette(window);"
                 "  color: palette(window-text)"
                 "}"
                 "DetailView .QFrame {"
                 "  border-top: 1px solid palette(dark)"
                 "}"

                 "DiffView {"
                 "  background: palette(light)"
                 "}"
                 "DiffView FileWidget {"
                 "  background: palette(mid)"
                 "}"
                 "DiffView HunkWidget {"
                 "  border-top: 1px solid palette(light)"
                 "}"
                 "DiffView HunkWidget QLabel {"
                 "  color: palette(bright-text)"
                 "}"

                 "FindWidget {"
                 "  background: palette(highlight)"
                 "}"
                 "FindWidget QToolButton {"
                 "  border: none;"
                 "  border-radius: 4px"
                 "}"
                 "FindWidget QToolButton:pressed {"
                 "  background: rgba(0, 0, 0, 20%)"
                 "}"
                 "FindWidget QLineEdit {"
                 "  border: none"
                 "}"

                 "Footer {"
                 "  background: palette(button);"
                 "  border: 1px solid palette(shadow);"
                 "  border-top: none"
                 "}"
                 "Footer QToolButton {"
                 "  border: 1px solid palette(shadow);"
                 "  border-top: none;"
                 "  border-left: none"
                 "}"

                 "LogView {"
                 "  selection-background-color: palette(mid)"
                 "}"

                 "MenuBar {"
                 "  background: %2;"
                 "  color: %3"
                 "}"

                 "QComboBox QListView {"
                 "  background: palette(base);"
                 "}"

                 "QToolTip {"
                 "  color: palette(text);"
                 "  background: palette(highlight);"
                 "  border: 1px solid palette(highlight)"
                 "}"

                 "TabBar::tab {"
                 "  border: none;"
                 "  border-right: 1px solid %5;"
                 "  background: %5;"
                 "  color: %6;"
                 "}"
                 "TabBar::tab:selected {"
                 "  background: %7;"
                 "}"

                 "ToolBar {"
                 "  border-top: none;"
                 "  border-bottom: 1px solid palette(dark)"
                 "}"
                 "ToolBar QToolButton {"
                 "  border: 1px solid palette(shadow)"
                 "}"
                 "ToolBar QToolButton:enabled:active:checked {"
                 "  background: %4"
                 "}"

                 "TreeWidget QColumnView {"
                 "  border-top: 1px solid palette(window);"
                 "  border-right: 1px solid palette(base);"
                 "  border-bottom: 1px solid palette(window)"
                 "}"

                 "CommitDetail QToolButton,"
                 "HunkWidget QToolButton {"
                 "  border: 1px solid palette(shadow);"
                 "  border-radius: 4px"
                 "}"

                 "CommitToolBar QToolButton {"
                 "  background: none"
                 "}"

                 "QTableView QPushButton {"
                 "  margin: 2px;"
                 "  padding: 6px"
                 "}"

                 "QWidget {"
                 "  %8"
                 "}";

  QVariantMap button = mMap.value("button").toMap();
  QVariantMap menubar = mMap.value("menubar").toMap();
  QVariantMap tabbar = mMap.value("tabbar").toMap();

  QString tabbarBase = tabbar.value("base").toString();
  if (tabbarBase.isEmpty())
    tabbarBase = "palette(dark)";

  QString tabbarText = tabbar.value("text").toString();
  if (tabbarText.isEmpty())
    tabbarText = "palette(text)";

  QString tabbarSelected = tabbar.value("selected").toString();
  if (tabbarSelected.isEmpty())
    tabbarSelected = "palette(window)";

  QString font;
  QVariantMap fontMap = mMap.value("font").toMap();

  QString fontValue = fontMap.value("family").toString();
  if (!fontValue.isEmpty())
    font += QString("font-family: \"%1\";").arg(fontValue);

  QVariant fontVariantValue = fontMap.value("size");
  bool ok;
  double fontNumValue = fontVariantValue.toDouble(&ok);
  if (ok) {
    font += QString("font-size: %1px;").arg(fontNumValue);
  } else {
    fontValue = fontVariantValue.toString();
    if (!fontValue.isEmpty())
      font += QString("font-size: %1;").arg(fontValue);
  }

  fontValue = fontMap.value("weight").toString();
  if (!fontValue.isEmpty())
    font += QString("font-weight: %1;").arg(fontValue);

  fontValue = fontMap.value("style").toString();
  if (!fontValue.isEmpty())
    font += QString("font-style: %1;").arg(fontValue);

  return text.arg(
      button.value("background").toMap().value("pressed").toString(),
      menubar.value("background").toString(), menubar.value("text").toString(),
      button.value("background").toMap().value("checked").toString(),
      tabbarBase, tabbarText, tabbarSelected, font);
}

void CustomTheme::polish(QPalette &palette) const {
  QVariantMap map = mMap.value("palette").toMap();
  setPaletteColors(palette, QPalette::Light, map.value("light"));
  setPaletteColors(palette, QPalette::Midlight, map.value("midlight"));
  setPaletteColors(palette, QPalette::Mid, map.value("middark"));
  setPaletteColors(palette, QPalette::Dark, map.value("dark"));
  setPaletteColors(palette, QPalette::Shadow, map.value("shadow"));

  QVariantMap widget = mMap.value("widget").toMap();
  setPaletteColors(palette, QPalette::Text, widget.value("text"));
  setPaletteColors(palette, QPalette::BrightText, widget.value("bright_text"));
  setPaletteColors(palette, QPalette::Base, widget.value("background"));
  setPaletteColors(palette, QPalette::AlternateBase, widget.value("alternate"));
  setPaletteColors(palette, QPalette::Highlight, widget.value("highlight"));
  setPaletteColors(palette, QPalette::HighlightedText,
                   widget.value("highlighted_text"));

  QVariantMap window = mMap.value("window").toMap();
  setPaletteColors(palette, QPalette::WindowText, window.value("text"));
  setPaletteColors(palette, QPalette::Window, window.value("background"));

  QVariantMap button = mMap.value("button").toMap();
  setPaletteColors(palette, QPalette::ButtonText, button.value("text"));
  setPaletteColors(palette, QPalette::Button, button.value("background"));

  QVariantMap link = mMap.value("link").toMap();
  setPaletteColors(palette, QPalette::Link, link.value("link"));
  setPaletteColors(palette, QPalette::LinkVisited, link.value("link_visited"));

  QVariantMap tooltip = mMap.value("tooltip").toMap();
  setPaletteColors(palette, QPalette::ToolTipText, tooltip.value("text"));
  setPaletteColors(palette, QPalette::ToolTipBase, tooltip.value("background"));
}

QColor CustomTheme::badge(BadgeRole role, BadgeState state) {
  QString roleKey;
  switch (role) {
    case BadgeRole::Foreground:
      roleKey = "foreground";
      break;

    case BadgeRole::Background:
      roleKey = "background";
      break;
  }

  QString stateKey;
  switch (state) {
    case BadgeState::Normal:
      stateKey = "normal";
      break;

    case BadgeState::Selected:
      stateKey = "selected";
      break;

    case BadgeState::Conflicted:
      stateKey = "conflicted";
      break;

    case BadgeState::Head:
      stateKey = "head";
      break;

    case BadgeState::Notification:
      stateKey = "notification";
      break;
    case BadgeState::Modified:
      stateKey = "modified";
      break;
    case BadgeState::Added:
      stateKey = "added";
      break;
    case BadgeState::Deleted:
      stateKey = "deleted";
      break;
    case BadgeState::Untracked:
      stateKey = "untracked";
      break;
    case BadgeState::Renamed:
      stateKey = "renamed";
      break;
  }

  QVariantMap badge = mMap.value("badge").toMap();
  QVariant variant = badge.value(roleKey);
  if (variant.canConvert<QColor>())
    return variant.value<QColor>();

  QVariantMap map = variant.toMap();
  if (!map.contains(stateKey))
    stateKey = "normal";

  return map.value(stateKey).value<QColor>();
}

QList<QColor> CustomTheme::branchTopologyEdges() {
  QVariantMap edge = mMap.value("graph").toMap();

  QList<QColor> colors;
  for (int i = 0; i < 15; i++) {
    QString name = QString("edge%1").arg(i + 1);
    colors.append(edge.value(name).toString());
  }

  return colors;
}

QColor CustomTheme::buttonChecked() {
  QVariantMap button = mMap.value("button").toMap();
  QVariantMap buttonText = button.value("text").toMap();
  if (buttonText.contains("checked"))
    return buttonText.value("checked").value<QColor>();
  return buttonText.value("default").value<QColor>();
}

QVariantMap CustomTheme::checkbox() const {
  return mMap.value("checkbox").toMap();
}

QPalette CustomTheme::commitList() {
  QVariantMap commitList = mMap.value("commits").toMap();

  QPalette palette;
  setPaletteColors(palette, QPalette::Text, commitList.value("text"));
  setPaletteColors(palette, QPalette::BrightText,
                   commitList.value("bright_text"));
  setPaletteColors(palette, QPalette::Base, commitList.value("background"));
  setPaletteColors(palette, QPalette::AlternateBase,
                   commitList.value("alternate"));
  setPaletteColors(palette, QPalette::Highlight, commitList.value("highlight"));
  setPaletteColors(palette, QPalette::HighlightedText,
                   commitList.value("highlighted_text"));
  setPaletteColors(palette, QPalette::WindowText,
                   commitList.value("highlighted_bright_text"));
  return palette;
}

QColor CustomTheme::commitEditor(CommitEditor color) {
  QVariantMap commitEditor = mMap.value("commiteditor").toMap();

  switch (color) {
    case CommitEditor::SpellError:
      return commitEditor.value("spellerror").value<QColor>();
    case CommitEditor::SpellIgnore:
      return commitEditor.value("spellignore").value<QColor>();
    case CommitEditor::LengthWarning:
      return commitEditor.value("lengthwarning").value<QColor>();
  }
  throw std::runtime_error("unreachable; value=" +
                           std::to_string(static_cast<int>(color)));
}

QColor CustomTheme::diff(Diff color) {
  QVariantMap diff = mMap.value("diff").toMap();

  switch (color) {
    case Diff::Ours:
      return diff.value("ours").value<QColor>();
    case Diff::Theirs:
      return diff.value("theirs").value<QColor>();
    case Diff::Addition:
      return diff.value("addition").value<QColor>();
    case Diff::Deletion:
      return diff.value("deletion").value<QColor>();
    case Diff::WordAddition:
      return diff.value("word_addition").value<QColor>();
    case Diff::WordDeletion:
      return diff.value("word_deletion").value<QColor>();
    case Diff::Plus:
      return diff.value("plus").value<QColor>();
    case Diff::Minus:
      return diff.value("minus").value<QColor>();
    case Diff::Note:
      return diff.value("note").value<QColor>();
    case Diff::Warning:
      return diff.value("warning").value<QColor>();
    case Diff::Error:
      return diff.value("error").value<QColor>();
  }
  throw std::runtime_error("unreachable; value=" +
                           std::to_string(static_cast<int>(color)));
}

QColor CustomTheme::heatMap(HeatMap color) {
  QVariantMap heatmap = mMap.value("blame").toMap();

  switch (color) {
    case HeatMap::Hot:
      return QColor(heatmap.value("hot").toString());
    case HeatMap::Cold:
      return QColor(heatmap.value("cold").toString());
  }
  throw std::runtime_error("unreachable; value=" +
                            std::to_string(static_cast<int>(color)));
}

QColor CustomTheme::remoteComment(Comment color) {
  QVariantMap comment = mMap.value("comment").toMap();

  switch (color) {
    case Comment::Background:
      return QColor(comment.value("background").toString());
    case Comment::Body:
      return QColor(comment.value("body").toString());
    case Comment::Author:
      return QColor(comment.value("author").toString());
    case Comment::Timestamp:
      return QColor(comment.value("timestamp").toString());
  }
  throw std::runtime_error("unreachable; value=" +
                            std::to_string(static_cast<int>(color)));
}

QColor CustomTheme::star() {
  return mMap.value("star").toMap().value("fill").value<QColor>();
}

#ifndef Q_OS_MAC
void CustomTheme::polishWindow(QWindow *window) const {
  Q_UNUSED(window)

  // FIXME: Change title bar color?
}
#endif

void CustomTheme::drawCloseButton(const QStyleOption *option,
                                  QPainter *painter) {
  qreal in = 3.5;
  qreal out = 8.0;
  QRect rect = option->rect;
  qreal x = rect.x() + (rect.width() / 2);
  qreal y = rect.y() + (rect.height() / 2);

  painter->save();
  painter->setRenderHints(QPainter::Antialiasing);

  // Draw background.
  if (option->state & QStyle::State_MouseOver) {
    painter->save();
    painter->setPen(Qt::NoPen);
    bool selected = (option->state & QStyle::State_Selected);
    painter->setBrush(QColor(selected ? QPalette().color(QPalette::Highlight)
                                      : QPalette().color(QPalette::Base)));
    QRectF background(x - out, y - out, 2 * out, 2 * out);
    painter->drawRoundedRect(background, 2.0, 2.0);
    painter->restore();
  }

  // Draw x.
  painter->setPen(QPen(QPalette().color(QPalette::WindowText), 1.5));
  painter->drawLine(QPointF(x - in, y - in), QPointF(x + in, y + in));
  painter->drawLine(QPointF(x - in, y + in), QPointF(x + in, y - in));
  painter->restore();
}

QDir CustomTheme::userDir(bool create, bool *exists) {
  QDir dir = Settings::userDir();

  if (create)
    dir.mkpath("themes");

  bool tmp = dir.cd("themes");
  if (exists)
    *exists = tmp;

  return dir;
}

bool CustomTheme::isValid(const QString &name) {
  QDir confDir = Settings::themesDir();
  if (QFileInfo(confDir.filePath(QString("%1.lua").arg(name))).exists())
    return true;

  bool exists = false;
  QDir dir = userDir(false, &exists);
  return exists &&
         QFileInfo(dir.filePath(QString("%1.lua").arg(name))).exists();
}
