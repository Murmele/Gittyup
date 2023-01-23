#ifndef COMMENTWIDGET_H
#define COMMENTWIDGET_H

#include "host/Account.h"
#include "Comment.h"
#include <QWidget>
#include <QVBoxLayout>

class CommentWidget : public QWidget {
public:
  CommentWidget(const Account::Comments &comments, QWidget *parent = nullptr)
      : QWidget(parent) {
    setStyleSheet("QWidget { background-color: transparent; border: none }");
    setContentsMargins(4, 4, 4, 4);

    QVBoxLayout *layout = new QVBoxLayout(this);
    foreach (const QDateTime &key, comments.keys())
      layout->addWidget(new Comment(key, comments.value(key), this));
  }
};

#endif // COMMENTWIDGET_H
