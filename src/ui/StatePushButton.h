//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef STATEPUSHBUTTON_H
#define STATEPUSHBUTTON_H

#include <QPushButton>

/*!
 */
class StatePushButton : public QPushButton {
  Q_OBJECT

public:
  StatePushButton(QString textChecked, QString textUnchecked,
                  QWidget *parent = nullptr);
  void setState(bool checked);
  bool toggleState();
  bool checked();

private:
  bool m_checked{false};
  QString m_textChecked{""};
  QString m_textUnchecked{""};
};
#endif // STATEPUSHBUTTON_H
