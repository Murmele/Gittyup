#include "FileLabel.h"
#include "DiffView.h"
#include "../RepoView.h"

#include <QPainter>
#include <QPainterPath>

FileLabel::FileLabel(const QString &name, bool submodule, QWidget *parent)
    : QWidget(parent), mName(name) {
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  QFont font = this->font();
  font.setBold(true);
  font.setPointSize(font.pointSize() + 3);
  if (submodule) {
    font.setUnderline(true);
    setCursor(Qt::PointingHandCursor);
  }

  setFont(font);
}

void FileLabel::setName(const QString &name) { mName = name; }

void FileLabel::setOldName(const QString &oldName) { mOldName = oldName; }

QSize FileLabel::sizeHint() const {
  QFontMetrics fm = fontMetrics();
  int width = fm.boundingRect(mName).width() + 2;
  if (!mOldName.isEmpty())
    width += fm.boundingRect(mOldName).width() + DiffViewStyle::kArrowWidth;
  return QSize(width, fm.lineSpacing());
}

void FileLabel::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QRect rect = this->rect();
  QFontMetrics fm = fontMetrics();

  if (!mOldName.isEmpty()) {
    // Draw old name.
    int max = (width() - DiffViewStyle::kArrowWidth) / 2;
    QString oldName = fm.elidedText(mOldName, Qt::ElideLeft, max);
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, oldName);
    rect.adjust(fm.boundingRect(oldName).width(), 0, 0, 0);

    // Draw arrow.
    int x1 = rect.x() + DiffViewStyle::kArrowMargin;
    int x2 =
        rect.x() + DiffViewStyle::kArrowWidth - DiffViewStyle::kArrowMargin;
    int y = rect.height() / 2;
    QPainterPath path;
    path.moveTo(x1, y);
    path.lineTo(x2, y);
    path.moveTo(x2 - 3, y - 3);
    path.lineTo(x2, y);
    path.lineTo(x2 - 3, y + 3);
    QPen pen = painter.pen();
    pen.setWidthF(1.5);
    painter.setPen(pen);
    painter.drawPath(path);
    rect.adjust(DiffViewStyle::kArrowWidth, 0, 0, 0);
  }

  QString name = fm.elidedText(mName, Qt::ElideLeft, rect.width());
  painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, name);
}

void FileLabel::mouseReleaseEvent(QMouseEvent *event) {
  if (!rect().contains(event->pos()))
    return;

  QUrl url;
  url.setScheme("submodule");
  url.setPath(mName);
  RepoView::parentView(this)->visitLink(url.toString());
}
