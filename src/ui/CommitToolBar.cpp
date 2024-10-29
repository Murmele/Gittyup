//
//          Copyright (c) 2017, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "CommitToolBar.h"
#include "CommitList.h"
#include "ContextMenuButton.h"
#include "RepoView.h"
#include "ConfigKeys.h"
#include "conf/Settings.h"
#include "git/Config.h"
#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QStyleOption>
#include <QToolButton>

namespace {

const QString kStyleSheet = "QToolBar {"
                            "  border: none"
                            "}"
                            "QToolButton {"
                            "  border: none;"
                            "  border-radius: 4px;"
                            "  padding-right: 12px"
                            "}";
template <typename T> struct SettingsEntry {
  QString key;
  T value;
};

template <typename T> using SettingsMap = QMap<QString, SettingsEntry<T>>;

template <typename T> class ToolButton : public QToolButton {
public:
  ToolButton(const SettingsMap<T> &map, CommitToolBar *parent, T defaultValue)
      : QToolButton(parent) {
    setPopupMode(QToolButton::InstantPopup);

    QMenu *menu = new QMenu(this);
    QActionGroup *actions = new QActionGroup(menu);

    RepoView *view = RepoView::parentView(parent);
    git::Config config = view->repo().appConfig();
    foreach (const QString &key, map.keys()) {
      QAction *action = menu->addAction(key);
      action->setCheckable(true);
      actions->addAction(action);

      const SettingsEntry<T> &entry = map.value(key);
      if (config.value<T>(entry.key, defaultValue) == entry.value) {
        action->setChecked(true);
        setText(action->text());
      }

      connect(action, &QAction::triggered, [parent, entry] {
        RepoView *view = RepoView::parentView(parent);
        git::Config config = view->repo().appConfig();
        config.setValue(entry.key, entry.value);
        emit parent->settingsChanged();
      });
    }

    setMenu(menu);
    connect(menu, &QMenu::triggered,
            [this](QAction *action) { setText(action->text()); });

#ifdef Q_OS_MAC
    QFont font = this->font();
    font.setPointSize(11);
    setFont(font);
#endif
  }
};

class Style : public QProxyStyle {
public:
  Style(QStyle *style) : QProxyStyle(style) {}

  void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget = nullptr) const {
    if (element != QStyle::PE_IndicatorArrowDown) {
      QProxyStyle::drawPrimitive(element, option, painter, widget);
      return;
    }

    const QRect &rect = option->rect;
    int x = rect.x() + rect.width() - 16;
    int y = rect.y();

    QPainterPath path;
    path.moveTo(5, 4);
    path.lineTo(8, 7);
    path.lineTo(11, 4);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(option->palette.buttonText(), 1.5));
    painter->drawPath(path.translated(x, y));
    painter->restore();
  }
};

} // namespace

CommitToolBar::CommitToolBar(QWidget *parent) : QToolBar(parent) {
  setStyle(new Style(style()));
  setStyleSheet(kStyleSheet);
  setToolButtonStyle(Qt::ToolButtonTextOnly);

  SettingsMap<int> refsMap;
  refsMap.insert(tr("Show All Branches"),
                 {ConfigKeys::kRefsKey, (int)CommitList::RefsFilter::AllRefs});
  refsMap.insert(
      tr("Show Selected Branch"),
      {ConfigKeys::kRefsKey, (int)CommitList::RefsFilter::SelectedRef});
  refsMap.insert(tr("Show Selected Branch, First Parent Only"),
                 {ConfigKeys::kRefsKey,
                  (int)CommitList::RefsFilter::SelectedRefIgnoreMerge});
  addWidget(
      new ToolButton<int>(refsMap, this, (int)CommitList::RefsFilter::AllRefs));

  SettingsMap<bool> sortMap;
  sortMap.insert(tr("Sort by Date"), {ConfigKeys::kSortKey, true});
  sortMap.insert(tr("Sort Topologically"), {ConfigKeys::kSortKey, false});
  addWidget(new ToolButton(sortMap, this, true));

  QWidget *spacer = new QWidget(this);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  addWidget(spacer);

  // Add context menu button.
  RepoView *view = RepoView::parentView(this);
  git::Config config = view->repo().appConfig();

  ContextMenuButton *button = new ContextMenuButton(this);
  addWidget(button);

  QMenu *menu = new QMenu(button);
  button->setMenu(menu);

  QAction *graph = menu->addAction(tr("Show Graph"));
  graph->setCheckable(true);
  graph->setChecked(config.value<bool>(ConfigKeys::kGraphKey, true));
  connect(graph, &QAction::triggered, [this](bool checked) {
    RepoView *view = RepoView::parentView(this);
    git::Config config = view->repo().appConfig();
    config.setValue(ConfigKeys::kGraphKey, checked);
    emit settingsChanged();
  });

  QAction *status = menu->addAction(tr("Show Clean Status"));
  status->setCheckable(true);
  status->setChecked(config.value<bool>(ConfigKeys::kStatusKey, true));
  connect(status, &QAction::triggered, [this](bool checked) {
    RepoView *view = RepoView::parentView(this);
    view->repo().appConfig().setValue(ConfigKeys::kStatusKey, checked);
    emit settingsChanged();
  });

  menu->addSeparator();

  QAction *compact = menu->addAction(tr("Compact Mode"));
  compact->setCheckable(true);
  compact->setChecked(Settings::instance()
                          ->value(Setting::Id::ShowCommitsInCompactMode)
                          .toBool());
  connect(compact, &QAction::triggered, [this](bool checked) {
    Settings::instance()->setValue(Setting::Id::ShowCommitsInCompactMode,
                                   checked);
    emit settingsChanged();
  });

  menu->addSeparator();

  QAction *author = menu->addAction(tr("Show Author"));
  author->setCheckable(true);
  author->setChecked(Settings::instance()
                         ->value(Setting::Id::ShowCommitsAuthor, true)
                         .toBool());
  connect(author, &QAction::triggered, [this](bool checked) {
    Settings::instance()->setValue(Setting::Id::ShowCommitsAuthor, checked);
    emit settingsChanged();
  });

  QAction *date = menu->addAction(tr("Show Date"));
  date->setCheckable(true);
  date->setChecked(
      Settings::instance()->value(Setting::Id::ShowCommitsDate, true).toBool());
  connect(date, &QAction::triggered, [this](bool checked) {
    Settings::instance()->setValue(Setting::Id::ShowCommitsDate, checked);
    emit settingsChanged();
  });

  QAction *id = menu->addAction(tr("Show Id"));
  id->setCheckable(true);
  id->setChecked(
      Settings::instance()->value(Setting::Id::ShowCommitsId, true).toBool());
  connect(id, &QAction::triggered, [this](bool checked) {
    Settings::instance()->setValue(Setting::Id::ShowCommitsId, checked);
    emit settingsChanged();
  });
}
