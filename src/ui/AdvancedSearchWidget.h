//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Bryan Williams
//

#ifndef ADVANCEDSEARCHWIDGET_H
#define ADVANCEDSEARCHWIDGET_H

#include <QComboBox>
#include <QWidget>

class Index;
class QFormLayout;
class QLineEdit;

class AdvancedSearchWidget : public QWidget {
  Q_OBJECT

public:
  AdvancedSearchWidget(QWidget *parent = nullptr);

  void exec(QLineEdit *parent, Index *index);

signals:
  void accepted(const QString &query);

protected:
  void hideEvent(QHideEvent *event) override;

private:
  void accept();

  void addLine(QFormLayout *layout);
  void addField(int field, const QString &text, const QString &tooltip);
  void addDateField(int field, const QString &text, const QString &tooltip);
  void addField(QWidget *widget, QLineEdit *lineEdit, int field,
                const QString &text, const QString &tooltip);

  QList<QLineEdit *> mLineEdits;
};

#endif
