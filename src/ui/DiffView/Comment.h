#ifndef COMMENT_H
#define COMMENT_H

#include <QTextEdit>

#include "host/Account.h"

class Comment : public QTextEdit {
public:
  Comment(const QDateTime &date, const Account::Comment &comment,
          QWidget *parent = nullptr);

  QSize minimumSizeHint() const override;
  QSize viewportSizeHint() const override;
};

#endif // COMMENT_H
