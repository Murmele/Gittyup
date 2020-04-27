//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ToolBar.h"
#include "History.h"
#include "MainWindow.h"
#include "RepoView.h"
#include "SearchField.h"
#include "app/Application.h"
#include "dialogs/PullRequestDialog.h"
#include "git/Branch.h"
#include "git/Commit.h"
#include <QAction>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionToolButton>
#include <QToolButton>
#include <QWindow>
#include <QtMath>

namespace {

const int kButtonWidth = 36;
const int kButtonHeight = 24;
const int kToolBarHeight = 32;
const QString kStarredQuery = "is:starred";
const QString kStyleSheet =
  "QToolButton {"
  "  border-radius: 4px;"
  "  padding: 0px 4px 0px 4px"
  "}"
  "QToolButton#first {"
  "  border-top-right-radius: 0px;"
  "  border-bottom-right-radius: 0px"
  "}"
  "QToolButton#middle {"
  "  border-left: none;"
  "  border-radius: 0px"
  "}"
  "QToolButton#last {"
  "  border-left: none;"
  "  border-top-left-radius: 0px;"
  "  border-bottom-left-radius: 0px"
  "}"
  "QToolButton::menu-indicator {"
  "  image: none"
  "}"
  "QToolButton::menu-button {"
  "  border: none;"
  "  width: 10px"
  "}"
  "QToolButton::menu-arrow {"
  "  image: none"
  "}";

class Spacer : public QWidget
{
public:
  Spacer(int width = -1, QWidget *parent = nullptr)
    : QWidget(parent), mWidth(width)
  {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    if (width < 0)
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  }

  QSize sizeHint() const override
  {
    return QSize(qMax(0, mWidth), kToolBarHeight);
  }

private:
  int mWidth;
};

class Button : public QToolButton
{
public:
  Button(QWidget *parent = nullptr)
    : QToolButton(parent)
  {}

  QSize sizeHint() const override
  {
    return QSize(kButtonWidth, kButtonHeight);
  }
};

class SidebarButton : public Button
{
public:
  enum Kind
  {
    Left,
    Right
  };

  SidebarButton(Kind kind, QWidget *parent = nullptr)
    : Button(parent), mKind(kind)
  {}

  void paintEvent(QPaintEvent *event)
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QColor color = opt.palette.buttonText().color();
    QColor light = (isEnabled() && isActiveWindow()) ? color.lighter() : color;

    QPainter painter(this);
    painter.setPen(QPen(color, 1.0));
    if (window()->windowHandle()->devicePixelRatio() > 1.0)
      painter.setRenderHint(QPainter::Antialiasing);

    qreal dx = 2.0;
    qreal x = width() / 2.0;
    qreal y = height() / 2.0;
    if (mKind == Right)
      dx = -dx; // invert

    painter.drawRect(QRectF(x - 8, y - 7, 16, 13));
    painter.drawLine(QLineF(x - dx, y - 7, x - dx, y + 6));

    qreal dx2x = 2 * dx;
    qreal dx3x = 3 * dx;
    painter.setPen(QPen(light, 1.0));
    painter.drawLine(QLineF(x - dx3x, y - 4, x - dx2x, y - 4));
    painter.drawLine(QLineF(x - dx3x, y - 2, x - dx2x, y - 2));
    painter.drawLine(QLineF(x - dx3x, y, x - dx2x, y));
  }

private:
  Kind mKind;
};

class HistoryButton : public Button
{
public:
  enum Kind
  {
    Prev,
    Next
  };

  HistoryButton(Kind kind, QWidget *parent = nullptr)
    : Button(parent), mKind(kind)
  {}

  QSize sizeHint() const override
  {
    return QSize(QToolButton::sizeHint().width(), kButtonHeight);
  }

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 1.5));
    painter.setRenderHint(QPainter::Antialiasing);

    qreal dx = 2.5;
    qreal dy = 5.0;
    qreal x = width() / 2.0;
    qreal y = height() / 2.0;
    if (mKind == Prev)
      dx = -dx; // invert

    QPainterPath path;
    path.moveTo(x - dx, y - dy);
    path.lineTo(x + dx, y);
    path.lineTo(x - dx, y + dy);

    painter.drawPath(path);
  }

private:
  Kind mKind;
};

class RemoteButton : public Button
{
  Q_OBJECT

public:
  enum Kind
  {
    Fetch,
    Pull,
    Push
  };

  RemoteButton(Kind kind, QWidget *parent = nullptr)
    : Button(parent), mKind(kind)
  {}

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 1.0));
    painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;

    QPainterPath path;
    if (mKind == Fetch) {
      path.moveTo(x + 8, y - 6);
      path.quadTo(x + 8, y - 2, x - 1, y - 2);
      path.lineTo(x - 1, y - 6);
      path.lineTo(x - 7, y);
      path.lineTo(x - 1, y + 6);
      path.lineTo(x - 1, y + 2);
      path.quadTo(x + 8, y + 2, x + 8, y - 6);
    } else if (mKind == Pull) {
      path.moveTo(x - 7, y);
      path.lineTo(x - 1, y - 6);
      path.lineTo(x - 1, y - 2);
      path.lineTo(x + 7, y - 2);
      path.lineTo(x + 7, y + 2);
      path.lineTo(x - 1, y + 2);
      path.lineTo(x - 1, y + 6);
      path.lineTo(x - 7, y);
    } else {
      path.moveTo(x + 7, y);
      path.lineTo(x + 1, y + 6);
      path.lineTo(x + 1, y + 2);
      path.lineTo(x - 7, y + 2);
      path.lineTo(x - 7, y - 2);
      path.lineTo(x + 1, y - 2);
      path.lineTo(x + 1, y - 6);
      path.lineTo(x + 7, y);
    }

    // Draw outline on high resolution displays.
    if (window()->windowHandle()->devicePixelRatio() > 1.0) {
      painter.drawPath(path);
    } else {
      painter.fillPath(path, opt.palette.buttonText());
    }

    if (popupMode() == QToolButton::MenuButtonPopup) {
      // Draw relative to lower right corner.
      qreal x = width() - 9.5;
      qreal y = height() - 6.5;

      QPainterPath path;
      path.moveTo(x, y);
      path.lineTo(x + 3, y + 3);
      path.lineTo(x + 6, y);

      painter.setPen(QPen(opt.palette.buttonText(), 1.5));
      painter.drawPath(path);
    }

    // Draw badge.
    if (mBadge > 0) {
#ifdef Q_OS_LINUX
      QFont font = painter.font();
      font.setPointSize(font.pointSize() - 2);
      painter.setFont(font);
#endif

      QString text = (mBadge > 999) ? tr("999+") : QString::number(mBadge);
      QFontMetrics fm = painter.fontMetrics();

      int w = fm.horizontalAdvance(text) + 8;
      QRect rect(width() - w - 2, 2, w, fm.lineSpacing() + 2);

      Theme *theme = Application::theme();
      auto state = Theme::BadgeState::Notification;
      QColor color = theme->badge(Theme::BadgeRole::Background, state);
      color.setAlphaF(0.6);
      painter.setBrush(color);
      painter.setPen(Qt::NoPen);
      painter.drawRoundedRect(rect, 6, 6);

      painter.setPen(theme->badge(Theme::BadgeRole::Foreground, state));
      painter.drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, text);
    }
  }

  void setBadge(int badge)
  {
    mBadge = badge;
    update();
  }

private:
  Kind mKind;
  int mBadge = 0;
};

class StashButton : public Button
{
public:
  enum Kind
  {
    Stash,
    Pop
  };

  StashButton(Kind kind, QWidget *parent = nullptr)
    : Button(parent), mKind(kind)
  {}

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 1.0));
    if (window()->windowHandle()->devicePixelRatio() > 1.0)
      painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;
    int offset = (mKind == Pop) ? 1 : 0;

    painter.drawRect(QRectF(x - 7, y - 6 - offset, 14, 4 - offset));
    painter.drawRect(QRectF(x - 6, y - 2, 12, 8));
    painter.drawLine(QLineF(x - 3, y, x + 3, y));
  }

private:
  Kind mKind;
};

class CheckButton : public Button
{
public:
  CheckButton(QWidget *parent = nullptr)
    : Button(parent)
  {}

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 1));
    painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;

    QPainterPath path;
    path.moveTo(x - 1, y + 2);
    path.lineTo(x - 3, y - 1);
    path.lineTo(x - 7, y - 1);
    path.lineTo(x - 2, y + 6);
    path.lineTo(x + 1, y + 6);
    path.lineTo(x + 7, y - 6);
    path.lineTo(x + 3, y - 6);
    path.closeSubpath();

    // Draw outline on high resolution displays.
    if (window()->windowHandle()->devicePixelRatio() > 1.0) {
      painter.drawPath(path);
    } else {
      painter.fillPath(path, opt.palette.buttonText());
    }
  }
};

class RefreshButton : public Button
{
  Q_OBJECT

public:
  RefreshButton(QWidget *parent = nullptr)
    : Button(parent)
  {
    setObjectName("RefreshButton");
    setToolTip(tr("Refresh"));
  }

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 1.5));
    painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;

    // Subtract a diagonal rectangle from the clip area.
    QPainterPath clip;
    clip.addRect(rect());

    QPainterPath path;
    path.moveTo(x - 8, y);
    path.lineTo(x + 8, y - 4);
    path.lineTo(x + 8, y);
    path.lineTo(x - 8, y + 4);
    path.closeSubpath();

    painter.setClipPath(clip.subtracted(path));
    painter.drawEllipse(QPointF(x, y), 6, 6);
    painter.setClipping(false);

    QPainterPath path1;
    path1.moveTo(x - 9, y - 1);
    path1.lineTo(x - 6, y + 2);
    path1.lineTo(x - 3, y - 1);
    path1.closeSubpath();
    painter.fillPath(path1, opt.palette.buttonText());

    QPainterPath path2;
    path2.moveTo(x + 9, y + 1);
    path2.lineTo(x + 6, y - 2);
    path2.lineTo(x + 3, y + 1);
    path2.closeSubpath();
    painter.fillPath(path2, opt.palette.buttonText());
  }
};

class PullRequestButton : public Button
{
  Q_OBJECT

public:
  PullRequestButton(QWidget *parent = nullptr)
    : Button(parent)
  {
    setObjectName("PullRequestButton");
    setToolTip(tr("Create Pull Request"));
  }

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 1.25));
    painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;

    QPainterPath path;
    path.addEllipse(x - 6, y - 7, 4, 4);
    path.addEllipse(x - 6, y + 3, 4, 4);
    path.addEllipse(x + 3, y, 4, 4);

    path.moveTo(x - 4, y + 3);
    path.lineTo(x - 4, y - 3);
    path.quadTo(x - 2, y + 2, x + 2, y + 2);
    painter.drawPath(path);
  }
};

class LogButton : public Button
{
public:
  LogButton(QWidget *parent = nullptr)
    : Button(parent)
  {}

  void paintEvent(QPaintEvent *event)
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QColor color = opt.palette.buttonText().color();
    QColor light = (isEnabled() && isActiveWindow()) ? color.lighter() : color;

    QPainter painter(this);
    painter.setPen(QPen(color, 1.0));
    if (window()->windowHandle()->devicePixelRatio() > 1.0)
      painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;

    painter.drawRect(QRectF(x - 8, y - 7, 16, 13));
    painter.drawLine(QLineF(x - 8, y, x + 8, y));

    painter.setPen(QPen(light, 1.0));
    painter.drawLine(QLineF(x - 6, y + 2, x + 6, y + 2));
    painter.drawLine(QLineF(x - 6, y + 4, x + 6, y + 4));
  }
};

class ModeButton : public Button
{
public:
  ModeButton(RepoView::ViewMode mode, QWidget *parent = nullptr)
    : Button(parent), mMode(mode)
  {}

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    Theme *theme = Application::theme();
    QColor color = (isEnabled() && isActiveWindow() && isChecked()) ?
      theme->buttonChecked() : opt.palette.color(QPalette::ButtonText);
    painter.setPen(QPen(color, 1.25));
    painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;

    if (mMode == RepoView::Diff) {
      // Subtract a diagonal rectangle from the clip area.
      QPainterPath clip;
      clip.addRect(rect());

      QPainterPath path;
      path.moveTo(x - 3, y - 4);
      path.lineTo(x + 5, y + 2);
      path.lineTo(x + 3, y + 4);
      path.lineTo(x - 5, y - 2);
      path.closeSubpath();

      painter.setClipPath(clip.subtracted(path));

      QPainterPath path1;
      path1.addEllipse(x + 4, y - 7, 4, 4);
      path1.addEllipse(x - 8, y + 3, 4, 4);

      path1.moveTo(x - 4, y + 3);
      path1.lineTo(x + 4, y - 3);
      painter.drawPath(path1);

      painter.setClipping(false);

      QPainterPath path2;
      path2.addEllipse(x - 8, y - 7, 4, 4);
      path2.addEllipse(x + 4, y + 3, 4, 4);

      path2.moveTo(x - 4, y - 3);
      path2.lineTo(x + 4, y + 3);
      painter.drawPath(path2);

    } else {
      QPainterPath path;
      path.addEllipse(x - 8, y - 7, 4, 4);
      path.addEllipse(x, y - 7, 4, 4);
      path.addEllipse(x, y + 3, 4, 4);

      path.moveTo(x - 3, y - 5);
      path.lineTo(x - 1, y - 5);
      path.moveTo(x + 5, y - 5);
      path.lineTo(x + 8, y - 5);

      path.moveTo(x + 2, y - 2);
      path.lineTo(x + 2, y + 2);
      path.moveTo(x + 5, y + 5);
      path.lineTo(x + 8, y + 5);
      painter.drawPath(path);
    }
  }

private:
  RepoView::ViewMode mMode;
};

class SettingsButton : public Button
{
public:
  SettingsButton(QWidget *parent = nullptr)
    : Button(parent)
  {}

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 2.25));
    painter.setRenderHint(QPainter::Antialiasing);

    qreal x = width() / 2.0;
    qreal y = height() / 2.0;

    // radii
    qreal inner = 4;
    qreal outer = 6.5;

    QPainterPath path;
    path.addEllipse(QPointF(x, y), inner, inner);
    for (int i = 0; i < 8; ++i) {
      qreal angle = i * M_PI_4; // in radians
      path.moveTo(x + qCos(angle) * inner, y + qSin(angle) * inner);
      path.lineTo(x + qCos(angle) * outer, y + qSin(angle) * outer);
    }

    painter.drawPath(path);
  }
};

class StarButton : public Button
{
public:
  StarButton(QWidget *parent = nullptr)
    : Button(parent)
  {
    setCheckable(true);
  }

  void paintEvent(QPaintEvent *event) override
  {
    Button::paintEvent(event);

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    QPainter painter(this);
    painter.setPen(QPen(opt.palette.buttonText(), 1.25));
    painter.setRenderHints(QPainter::Antialiasing);

    // Calculate outer radius and vertices.
    qreal r = 8.0;
    qreal x = width() / 2.0;
    qreal y = (height() / 2.0) + 1.0;
    qreal x1 = r * qCos(M_PI / 10.0);
    qreal y1 = -r * qSin(M_PI / 10.0);
    qreal x2 = r * qCos(17.0 * M_PI / 10.0);
    qreal y2 = -r * qSin(17.0 * M_PI / 10.0);

    // Calculate inner radius and verices.
    qreal xi = ((y1 + r) * x2) / (y2 + r);
    qreal ri = qSqrt(qPow(xi, 2.0) + qPow(y1, 2.0));
    qreal xi1 = ri * qCos(3.0 * M_PI / 10.0);
    qreal yi1 = -ri * qSin(3.0 * M_PI / 10.0);
    qreal xi2 = ri * qCos(19.0 * M_PI / 10.0);
    qreal yi2 = -ri * qSin(19.0 * M_PI / 10.0);

    QPolygonF polygon({
      QPointF(0, -r),
      QPointF(xi1, yi1),
      QPointF(x1, y1),
      QPointF(xi2, yi2),
      QPointF(x2, y2),
      QPointF(0, ri),
      QPointF(-x2, y2),
      QPointF(-xi2, yi2),
      QPointF(-x1, y1),
      QPointF(-xi1, yi1)
    });

    if (isChecked())
      painter.setBrush(Application::theme()->star());
    painter.drawPolygon(polygon.translated(x, y));
  }
};

class SegmentedButton : public QWidget
{
public:
  SegmentedButton(QWidget *parent = nullptr)
    : QWidget(parent)
  {
    mLayout = new QHBoxLayout(this);
    mLayout->setContentsMargins(0,0,0,0);
    mLayout->setSpacing(0);
  }

  void addButton(
    QAbstractButton *button,
    const QString &text = QString(),
    bool checkable = false)
  {
    button->setToolTip(text);
    button->setCheckable(checkable);

    mLayout->addWidget(button);
    mButtons.addButton(button, mButtons.buttons().size());

    if (mButtons.buttons().size() > 1) {
      mButtons.buttons().first()->setObjectName("first");
      mButtons.buttons().last()->setObjectName("last");
    }

    for (int i = 1; i < mButtons.buttons().size() - 1; ++i)
      mButtons.buttons().at(i)->setObjectName("middle");
  }

  const QButtonGroup *buttonGroup() const
  {
    return &mButtons;
  }

private:
  QHBoxLayout *mLayout;
  QButtonGroup mButtons;
};

} // anon. namespace

ToolBar::ToolBar(MainWindow *parent)
  : QToolBar(parent)
{
  Q_ASSERT(parent);

  setMovable(false);
  setObjectName("toolbar");
  setStyleSheet(kStyleSheet);

  // Disable the built-in context menu.
  setContextMenuPolicy(Qt::PreventContextMenu);

  addWidget(new Spacer(4, this));

  SidebarButton *sidebarButton = new SidebarButton(SidebarButton::Left, this);
  sidebarButton->setToolTip(tr("Show repository sidebar"));
  addWidget(sidebarButton);
  connect(sidebarButton, &QAbstractButton::clicked, [parent] {
    parent->setSideBarVisible(!parent->isSideBarVisible());
  });

  addWidget(new Spacer(4, this));

  SegmentedButton *historyButton = new SegmentedButton(this);
  addWidget(historyButton);

  mPrevButton = new HistoryButton(HistoryButton::Prev, historyButton);
  mPrevButton->setEnabled(false);
  historyButton->addButton(mPrevButton, tr("Previous"));
  connect(mPrevButton, &QAbstractButton::clicked, [this] {
    currentView()->history()->prev();
  });

  QMenu *prevMenu = new QMenu(mPrevButton);
  mPrevButton->setMenu(prevMenu);
  connect(prevMenu, &QMenu::triggered, [this](QAction *action) {
    currentView()->history()->setIndex(action->data().toInt());
  });

  mNextButton = new HistoryButton(HistoryButton::Next, historyButton);
  mNextButton->setEnabled(false);
  historyButton->addButton(mNextButton, tr("Next"));
  connect(mNextButton, &QAbstractButton::clicked, [this] {
    currentView()->history()->next();
  });

  QMenu *nextMenu = new QMenu(mNextButton);
  mNextButton->setMenu(nextMenu);
  connect(nextMenu, &QMenu::triggered, [this](QAction *action) {
    currentView()->history()->setIndex(action->data().toInt());
  });

  addWidget(new Spacer(4, this));

  SegmentedButton *remote = new SegmentedButton(this);
  addWidget(remote);

  mFetchButton = new RemoteButton(RemoteButton::Fetch, remote);
  remote->addButton(mFetchButton, tr("Fetch"));
  connect(mFetchButton, &Button::clicked, [this] {
    currentView()->fetch();
  });

  mPullButton = new RemoteButton(RemoteButton::Pull, remote);
  mPullButton->setPopupMode(QToolButton::MenuButtonPopup);
  remote->addButton(mPullButton, tr("Pull"));

  // Add pull button menu.
  QMenu *pullMenu = new QMenu(mPullButton);
  mPullButton->setMenu(pullMenu);

  QAction *mergeAction = pullMenu->addAction("Merge");
  connect(mergeAction, &QAction::triggered, [this] {
    currentView()->pull(RepoView::Merge);
  });

  QAction *rebaseAction = pullMenu->addAction("Rebase");
  connect(rebaseAction, &QAction::triggered, [this] {
    currentView()->pull(RepoView::Rebase);
  });

  connect(mPullButton, &Button::clicked, [this] {
    currentView()->pull();
  });

  mPushButton = new RemoteButton(RemoteButton::Push, remote);
  remote->addButton(mPushButton, tr("Push"));
  connect(mPushButton, &Button::clicked, [this] {
    currentView()->push();
  });

  addWidget(new Spacer(4, this));

  mCheckoutButton = new CheckButton(this);
  mCheckoutButton->setToolTip(tr("Checkout"));
  addWidget(mCheckoutButton);
  connect(mCheckoutButton, &Button::clicked, [this] {
    currentView()->promptToCheckout();
  });

  addWidget(new Spacer(4, this));

  SegmentedButton *stashButtons = new SegmentedButton(this);
  addWidget(stashButtons);

  mStashButton = new StashButton(StashButton::Stash, stashButtons);
  mStashButton->setEnabled(false);
  stashButtons->addButton(mStashButton, tr("Stash"));
  connect(mStashButton, &Button::clicked, [this] {
    currentView()->promptToStash();
  });

  mStashPopButton = new StashButton(StashButton::Pop, stashButtons);
  stashButtons->addButton(mStashPopButton, tr("Pop Stash"));
  connect(mStashPopButton, &Button::clicked, [this] {
    currentView()->popStash();
  });

  addWidget(new Spacer(4, this));

  mRefreshButton = new RefreshButton(this);
  addWidget(mRefreshButton);
  connect(mRefreshButton, &Button::clicked, [this] {
    currentView()->refresh();
  });

  if (!qgetenv("GITAHEAD_OAUTH").isEmpty()) {
    addWidget(new Spacer(4, this));

    mPullRequestButton = new PullRequestButton(this);
    addWidget(mPullRequestButton);
    connect(mPullRequestButton, &Button::clicked, [this] {
      PullRequestDialog *dialog = new PullRequestDialog(currentView());
      dialog->open();
    });
  }

  addWidget(new Spacer(-1, this));

  mConfigButton = new SettingsButton(this);
  mConfigButton->setToolTip(tr("Configure Settings"));
  addWidget(mConfigButton);
  connect(mConfigButton, &Button::clicked, [this] {
    currentView()->configureSettings();
  });

  addWidget(new Spacer(4, this));

  mLogButton = new LogButton(this);
  mLogButton->setToolTip(tr("Show Log"));
  addWidget(mLogButton);
  connect(mLogButton, &Button::clicked, [this] {
    RepoView *view = this->currentView();
    view->setLogVisible(!view->isLogVisible());
  });

  addWidget(new Spacer(4, this));

  SegmentedButton *mode = new SegmentedButton(this);
  mModeGroup = mode->buttonGroup();

  ModeButton *diff = new ModeButton(RepoView::Diff, mode);
  mode->addButton(diff, tr("Diff View"), true);
  diff->setChecked(true);

  ModeButton *tree = new ModeButton(RepoView::Tree, mode);
  mode->addButton(tree, tr("Tree View"), true);

  ModeButton *alternativeTree = new ModeButton(RepoView::Tree, mode);
  mode->addButton(alternativeTree, tr("Double Tree View"), true);

  addWidget(mode);

  using Signal = void (QButtonGroup::*)(int);
  auto signal = static_cast<Signal>(&QButtonGroup::buttonClicked);
  connect(mModeGroup, signal, [this](int index) {
    currentView()->setViewMode(static_cast<RepoView::ViewMode>(index));
  });

  addWidget(new Spacer(4, this));

  mStarButton = new StarButton(this);
  mStarButton->setToolTip(tr("Show Starred Commits"));
  addWidget(mStarButton);

  addWidget(new Spacer(4, this));

  mSearchField = new SearchField(this);
  addWidget(mSearchField);

#if 0
  SidebarButton *searchButton = new SidebarButton(SidebarButton::Right, this);
  addWidget(searchButton);
  connect(searchButton, &SidebarButton::clicked, [] {
    // ...
  });
#endif

  addWidget(new Spacer(4, this));

  // Hook up star button to search field.
  connect(mStarButton, &QToolButton::toggled, [this](bool checked) {
    QStringList terms = mSearchField->text().split(QRegExp("\\s+"));
    if (checked) {
      terms.append(kStarredQuery);
    } else {
      terms.removeAll(kStarredQuery);
    }

    mSearchField->setText(terms.join(' '));
  });

  connect(mSearchField, &SearchField::textChanged, [this](const QString &text) {
    QSignalBlocker blocker(mStarButton);
    (void) blocker;

    QStringList terms = mSearchField->text().split(QRegExp("\\s+"));
    mStarButton->setChecked(terms.contains(kStarredQuery));
  });
}

void ToolBar::updateButtons(int ahead, int behind)
{
  updateRemote(ahead, behind);
  updateHistory();
  updateStash();
  updateView();
  updateSearch();

  RepoView *view = currentView();
  mRefreshButton->setEnabled(view);
  if (mPullRequestButton)
    mPullRequestButton->setEnabled(view);
  mCheckoutButton->setEnabled(view && !view->repo().isBare());
}

void ToolBar::updateRemote(int ahead, int behind)
{
  static_cast<RemoteButton *>(mPushButton)->setBadge(ahead);
  static_cast<RemoteButton *>(mPullButton)->setBadge(behind);

  RepoView *view = currentView();
  mFetchButton->setEnabled(view);
  mPullButton->setEnabled(view && !view->repo().isBare());
  mPushButton->setEnabled(view);
}

void ToolBar::updateHistory()
{
  RepoView *view = currentView();
  History *history = view ? view->history() : nullptr;
  mPrevButton->setEnabled(history && history->hasPrev());
  mNextButton->setEnabled(history && history->hasNext());

  if (history) {
    history->updatePrevMenu(mPrevButton->menu());
    history->updateNextMenu(mNextButton->menu());
  }
}

void ToolBar::updateStash()
{
  RepoView *view = currentView();
  mStashButton->setEnabled(view && view->isWorkingDirectoryDirty());
  mStashPopButton->setEnabled(view && view->repo().stashRef().isValid());
}

void ToolBar::updateView()
{
  RepoView *view = currentView();
  mConfigButton->setEnabled(view);
  mLogButton->setEnabled(view);
  mModeGroup->button(RepoView::Diff)->setEnabled(view);
  mModeGroup->button(RepoView::Tree)->setEnabled(view);
  mModeGroup->button(RepoView::DoubleTree)->setEnabled(view);

  if (view) {
    bool visible = view->isLogVisible();
    mLogButton->setToolTip(visible ? tr("Hide Log") : tr("Show Log"));
    mModeGroup->button(view->viewMode())->setChecked(true);
  }
}

void ToolBar::updateSearch()
{
  RepoView *view = currentView();
  mStarButton->setEnabled(view);
  mSearchField->setEnabled(view);
}

RepoView *ToolBar::currentView() const
{
  return static_cast<MainWindow *>(parent())->currentView();
}

#include "ToolBar.moc"
