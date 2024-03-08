#include "HunkHeader.h"
#include "DiffView.h"
#include "../RepoView.h"

#include <QPainter>
#include <QPainterPath>

HunkHeader::HunkHeader(const QString &name, bool submodule, QWidget *parent)
    : QWidget(parent), mName(name) {
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void HunkHeader::setName(const QString &name) { mName = name; }

void HunkHeader::setOldName(const QString &oldName) { mOldName = oldName; }

QSize HunkHeader::sizeHint() const {
  QFontMetrics fm = fontMetrics();
  int width = fm.boundingRect(mName).width() + 2;
  if (!mOldName.isEmpty())
    width += fm.boundingRect(mOldName).width() + DiffViewStyle::kArrowWidth;
  return QSize(width, fm.lineSpacing());
}

void HunkHeader::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  QFontMetrics fm = fontMetrics();
  QRect rect = fm.boundingRect(0, 0, this->rect().width(), 300,
                               Qt::AlignLeft | Qt::ElideRight, mName);
  painter.drawText(rect, Qt::AlignLeft | Qt::ElideRight, mName);
}

void HunkHeader::mouseReleaseEvent(QMouseEvent *event) {
  if (!rect().contains(event->pos()))
    return;

  QUrl url;
  url.setScheme("submodule");
  url.setPath(mName);
  RepoView::parentView(this)->visitLink(url.toString());
}
