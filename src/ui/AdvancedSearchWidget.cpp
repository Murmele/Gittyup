//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#include "AdvancedSearchWidget.h"
#include "IndexCompleter.h"
#include "app/Application.h"
#include "index/Index.h"
#include "index/Query.h"
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDesktopWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QStringListModel>
#include <QtConcurrent>

namespace {

const char *kFieldProp = "field";
const QString kParenFmt = "(%1)";
const QString kFieldFmt = "%1:%2";

} // anon. namespace

class DateEdit : public QComboBox
{
public:
  DateEdit(QWidget *parent)
    : QComboBox(parent)
  {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setEditable(true);

    mCalendar = new QCalendarWidget(this);
    mCalendar->setWindowFlags(Qt::Popup);
    mCalendar->setVisible(false);

    connect(mCalendar, &QCalendarWidget::clicked, this, [this](const QDate &date) {
      setCurrentText(date.toString(Index::dateFormat()));
      hidePopup();
    });
  }

  void showPopup() override
  {
    mCalendar->setVisible(true);

    QPoint pos = mapToGlobal(rect().bottomLeft());
    QRect screen = QGuiApplication::screenAt(pos)->availableGeometry();

    QSize size = mCalendar->sizeHint();
    if (pos.x() + size.width() > screen.right())
      pos.setX(screen.right() - size.width());
    pos.setX(qMax(pos.x(), screen.left()));
    mCalendar->move(pos);
  }

  void hidePopup() override
  {
    mCalendar->setVisible(false);
    mCalendar->setSelectedDate(QDate::currentDate());
  }

private:
  QCalendarWidget *mCalendar;
};

AdvancedSearchWidget::AdvancedSearchWidget(QWidget *parent)
  : QWidget(parent, Qt::Popup)
{
  setStyleSheet("QLabel { color: palette(bright-text) }");

  QFormLayout *layout = new QFormLayout(this);
  layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  // name, email, message
  addField(Index::Author, tr("Author:"), tr("Author name"));
  addField(Index::Email, tr("Email:"), tr("Author email"));
  addField(Index::Message, tr("Message:"), tr("Commit message"));
  addLine(layout);

  // date
  addDateField(Index::Date, tr("Date:"), tr("Specific commit date"));
  addDateField(Index::After, tr("After:"), tr("Commits after date"));
  addDateField(Index::Before, tr("Before:"), tr("Commits before date"));
  addLine(layout);

  // file, path, scope
  addField(Index::File, tr("File:"), tr("File name"));
  addField(Index::Path, tr("Path:"), tr("File path"));
  addField(Index::Scope, tr("Scope:"), tr("Hunk header text"));

  addLine(layout);

  // context, addition, deletion
  addField(Index::Context, tr("Context:"), tr("Diff context (white)"));
  addField(Index::Addition, tr("Addition:"), tr("Diff addition (green)"));
  addField(Index::Deletion, tr("Deletion:"), tr("Diff deletion (red)"));

  addLine(layout);

  // comment, string, identifier
  addField(Index::Comment, tr("Comment:"), tr("Source code comment"));
  addField(Index::String, tr("String:"), tr("Source code string literal"));
  addField(Index::Identifier, tr("Identifier:"), tr("Source code identifier"));

  QPushButton *searchButton = new QPushButton(tr("Search"), this);
  QHBoxLayout *buttonLayout = new QHBoxLayout;
  buttonLayout->addStretch();
  buttonLayout->addWidget(searchButton);
  layout->addRow(buttonLayout);
  connect(searchButton, &QPushButton::clicked,
          this, &AdvancedSearchWidget::accept);

  QShortcut *accept = new QShortcut(tr("Return"), this);
  connect(accept, &QShortcut::activated,
          this, &AdvancedSearchWidget::accept);
}

void AdvancedSearchWidget::exec(QLineEdit *parent, Index *index)
{
  resize(parent->width(), sizeHint().height());
  move(parent->mapToGlobal(QPoint(0, parent->height() + 2)));

  // Load initial values from query.
  // FIXME: Phrase queries are lossy.
  QMap<Index::Field,QStringList> map;
  if (QueryRef query = Query::parseQuery(parent->text())) {
    foreach (const Index::Term &term, query->terms())
      map[term.field].append(term.text);
  }

  // Reset fields.
  foreach (QLineEdit *lineEdit, mLineEdits) {
    QVariant var = lineEdit->property(kFieldProp);
    Index::Field field = static_cast<Index::Field>(var.toInt());
    lineEdit->setText(map.value(field).join(' '));
  }

  // Load completion data in the background.
  QtConcurrent::run([this, index] {
    QMap<Index::Field,QStringList> fields = index->fieldMap();
    foreach (QLineEdit *lineEdit, mLineEdits) {
      QVariant var = lineEdit->property(kFieldProp);
      Index::Field field = static_cast<Index::Field>(var.toInt());
      QAbstractItemModel *model = lineEdit->completer()->model();
      if (QStringListModel *list = qobject_cast<QStringListModel *>(model))
        list->setStringList(fields.value(field));
    }
  });

  show();
}

void AdvancedSearchWidget::hideEvent(QHideEvent *event)
{
  // Clear completion data.
  foreach (QLineEdit *lineEdit, mLineEdits) {
    QAbstractItemModel *model = lineEdit->completer()->model();
    if (QStringListModel *list = qobject_cast<QStringListModel *>(model))
      list->setStringList(QStringList());
  }

  QWidget::hideEvent(event);
}

void AdvancedSearchWidget::accept()
{
  QStringList fields;
  foreach (QLineEdit *lineEdit, mLineEdits) {
    QString text = lineEdit->text();
    if (text.isEmpty())
      continue;

    // Enclose queries with embedded spaces in parentheses.
    if (text.contains(' ') && (!text.startsWith('"') || !text.endsWith('"')))
      text = kParenFmt.arg(text);

    QVariant var = lineEdit->property(kFieldProp);
    Index::Field field = static_cast<Index::Field>(var.toInt());
    fields.append(kFieldFmt.arg(Index::fieldName(field), text));
  }

  emit accepted(fields.join(' '));

  hide();
}

void AdvancedSearchWidget::addLine(QFormLayout *layout)
{
  QFrame *line = new QFrame(this);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  layout->addRow(line);
}

void AdvancedSearchWidget::addField(
  int field,
  const QString &text,
  const QString &tooltip)
{
  QLineEdit *lineEdit = new QLineEdit(this);
  QStringListModel *model = new QStringListModel(lineEdit);
  lineEdit->setCompleter(new IndexCompleter(model, lineEdit));
  addField(lineEdit, lineEdit, field, text, tooltip);
}

void AdvancedSearchWidget::addDateField(
  int field,
  const QString &text,
  const QString &tooltip)
{
  DateEdit *dateEdit = new DateEdit(this);
  addField(dateEdit, dateEdit->lineEdit(), field, text, tooltip);
}

void AdvancedSearchWidget::addField(
  QWidget *widget,
  QLineEdit *lineEdit,
  int field,
  const QString &text,
  const QString &tooltip)
{
  lineEdit->setProperty(kFieldProp, field);
  lineEdit->setClearButtonEnabled(true);
  mLineEdits.append(lineEdit);

  QLabel *label = new QLabel(text, this);
  label->setToolTip(tooltip);
  label->setCursor(Qt::WhatsThisCursor);  

  static_cast<QFormLayout *>(layout())->addRow(label, widget);
}
