//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "IndexCompleter.h"
#include "MainWindow.h"
#include "RepoView.h"
#include "SearchField.h"
#include "index/Index.h"
#include <QAbstractListModel>
#include <QEvent>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QRegularExpression>
#include <QToolButton>

namespace {

const QString kButtonStyle = "QPushButton {"
                             "  text-align: left;"
                             "  padding: 3px 3px 3px 3px;"
                             "  background: palette(base);"
#ifndef Q_OS_MAC
                             "  border: 1px solid palette(mid);"
#endif
                             "  border-top: 1px solid palette(dark)"
                             "}"
                             "QPushButton:pressed {"
                             "  background: palette(dark)"
                             "}";

const QRegularExpression kWsRe("\\s");

QString word(const QString &text, int &index) {
  index = text.lastIndexOf(kWsRe, qMax(0, index - 1)) + 1;
  return text.mid(index, text.indexOf(kWsRe, index) - index);
}

class Model : public QAbstractListModel {
public:
  Model(MainWindow *window) : QAbstractListModel(window), mWindow(window) {}

  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    if (!index.isValid())
      return QVariant();

    switch (role) {
      case Qt::EditRole:
      case Qt::DisplayRole:
        return dict().at(index.row()).key;

      default:
        return QVariant();
    }
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    return mWindow->repoCount() ? dict().size() : 0;
  }

private:
  const Index::Dictionary &dict() const {
    return mWindow->currentView()->index()->dict();
  }

  MainWindow *mWindow;
};

class Popup : public QListView {
  Q_OBJECT

public:
  Popup(QLineEdit *parent = nullptr) : QListView(parent) {
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setTextElideMode(Qt::ElideLeft);
    setUniformItemSizes(true);

    if (SearchField *field = qobject_cast<SearchField *>(parent)) {
      mButton = new QPushButton(tr("Show Advanced Search"), this);
      mButton->setStyleSheet(kButtonStyle);
      mButton->setFlat(true);
      connect(mButton, &QPushButton::clicked, [this, field] {
        hide();
        field->advancedButton()->click();
      });

      setViewportMargins(0, 0, 0, mButton->sizeHint().height());
    }
  }

  void adjustLayout(int maxVisibleItems) {
    if (!mButton)
      return;

    int height = mButton->sizeHint().height();
    int y = sizeHintForRow(0) * qMin(maxVisibleItems, model()->rowCount()) + 4;
    mButton->setGeometry(0, y, width(), height);
    setMinimumHeight(y + height);
  }

private:
  QPushButton *mButton = nullptr;
};

} // namespace

IndexCompleter::IndexCompleter(MainWindow *window, QLineEdit *parent)
    : IndexCompleter(new Model(window), parent) {}

IndexCompleter::IndexCompleter(QAbstractItemModel *model, QLineEdit *parent)
    : QCompleter(model, parent) {
  setCaseSensitivity(Qt::CaseInsensitive);
  setModelSorting(QCompleter::CaseInsensitivelySortedModel);

  Popup *popup = new Popup(parent);
  setPopup(popup);

  // Watch for popup show events.
  popup->installEventFilter(this);
}

bool IndexCompleter::eventFilter(QObject *watched, QEvent *event) {
  if (event->type() == QEvent::Show)
    mPos = -1;

  return QCompleter::eventFilter(watched, event);
}

QString IndexCompleter::pathFromIndex(const QModelIndex &index) const {
  QLineEdit *field = static_cast<QLineEdit *>(parent());
  if (!field->isEnabled())
    return QString();

  if (mPos < 0)
    mPos = field->cursorPosition();

  QString text = field->text();
  int len = word(text, mPos).length();
  return text.replace(mPos, len, index.data(completionRole()).toString());
}

QStringList IndexCompleter::splitPath(const QString &path) const {
  QLineEdit *field = static_cast<QLineEdit *>(parent());
  if (!field->isEnabled())
    return QStringList();

  // This is an egregious hack, but there doesn't
  // seem to be any reasonable place to hook in.
  static_cast<Popup *>(popup())->adjustLayout(maxVisibleItems());

  int pos = field->cursorPosition();
  QString term = word(path, pos);
  return QStringList(!term.isEmpty() ? term : path);
}

#include "IndexCompleter.moc"
