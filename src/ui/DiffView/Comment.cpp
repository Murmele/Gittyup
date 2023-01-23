#include "Comment.h"
#include "app/Application.h"
#include "DiffView.h"

#include <QScrollBar>

Comment::Comment(const QDateTime &date, const Account::Comment &comment,
                 QWidget *parent)
    : QTextEdit(parent) {
  setReadOnly(true);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QTextCursor cursor = textCursor();
  Theme *theme = Application::theme();

  QTextCharFormat author;
  author.setForeground(theme->remoteComment(Theme::Comment::Author));
  author.setFontWeight(QFont::Bold);
  cursor.setCharFormat(author);
  cursor.insertText(comment.author + " ");

  QTextCharFormat timestamp;
  timestamp.setForeground(theme->remoteComment(Theme::Comment::Timestamp));
  cursor.setCharFormat(timestamp);
  cursor.insertText(date.toString(Qt::DefaultLocaleLongDate));

  QTextBlockFormat indent;
  indent.setLeftMargin(fontMetrics().horizontalAdvance(' ') *
                       DiffViewStyle::kIndent);
  cursor.insertBlock(indent);

  QTextCharFormat body;
  body.setForeground(theme->remoteComment(Theme::Comment::Body));
  cursor.setCharFormat(body);
  cursor.insertText(comment.body);

  connect(verticalScrollBar(), &QScrollBar::rangeChanged,
          [this] { updateGeometry(); });
}

QSize Comment::minimumSizeHint() const { return QSize(); }

QSize Comment::viewportSizeHint() const { return document()->size().toSize(); }
