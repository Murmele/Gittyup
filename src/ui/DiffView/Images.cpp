#include "Images.h"
#include "git/Repository.h"
#include "app/Application.h"

#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QFileIconProvider>
#include <QLabel>

//#############################################################################
//###########     Image     ###################################################
//#############################################################################

_Images::Image::Image(const QPixmap &pixmap, QWidget *parent)
    : QWidget(parent), mPixmap(pixmap) {
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
}

QSize _Images::Image::sizeHint() const { return mPixmap.size(); }

bool _Images::Image::hasHeightForWidth() const { return true; }

int _Images::Image::heightForWidth(int w) const {
  QSize size = sizeHint();
  int width = size.width();
  int height = size.height();
  return (w >= width) ? height : height * (w / (qreal)width);
}

void _Images::Image::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.drawPixmap(rect(), mPixmap);
}

//#############################################################################
//###########     Arrow     ###################################################
//#############################################################################

_Images::Arrow::Arrow(QWidget *parent) : QWidget(parent) {
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
}

QSize _Images::Arrow::sizeHint() const { return QSize(100, 100); }

void _Images::Arrow::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  int x = width() / 2;
  int y = height() / 2;

  QSize size = sizeHint();
  int width = size.width();
  int height = size.height();

  QPainterPath arrow;
  arrow.moveTo(x - width * 0.40, y - height * 0.15);
  arrow.lineTo(x + width * 0.05, y - height * 0.15);
  arrow.lineTo(x + width * 0.05, y - height * 0.27);
  arrow.lineTo(x + width * 0.40, y);
  arrow.lineTo(x + width * 0.05, y + height * 0.27);
  arrow.lineTo(x + width * 0.05, y + height * 0.15);
  arrow.lineTo(x - width * 0.40, y + height * 0.15);
  arrow.closeSubpath();

  QPalette palette = Application::palette();
  painter.setPen(QPen(palette.brightText(), 2.0));
  painter.setBrush(palette.button());
  painter.drawPath(arrow);
}

//#############################################################################
//###########     Images     ##################################################
//#############################################################################

Images::Images(const git::Patch patch, bool lfs, QWidget *parent)
    : QWidget(parent), mPatch(patch) {
  int size = 0;
  QPixmap newFile = loadPixmap(git::Diff::NewFile, size, lfs);
  if (newFile.isNull()) {
    QString path = mPatch.repo().workdir().filePath(mPatch.name());
    size = QFileInfo(path).size();
    newFile.load(path);
  }

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(6, 4, 8, 4);

  int beforeSize = 0;
  QPixmap before = loadPixmap(git::Diff::OldFile, beforeSize, lfs);
  if (!before.isNull())
    layout->addLayout(imageLayout(before, beforeSize), 1);

  if (!before.isNull() && !newFile.isNull())
    layout->addWidget(new _Images::Arrow(this));

  if (!newFile.isNull())
    layout->addLayout(imageLayout(newFile, size), 1);
  layout->addStretch();
}

QPixmap Images::loadPixmap(git::Diff::File type, int &size, bool lfs) {
  git::Blob blob = mPatch.blob(type);
  if (!blob.isValid())
    return QPixmap();

  QByteArray data = blob.content();

  if (lfs)
    data = mPatch.repo().lfsSmudge(data, mPatch.name());

  size = data.length();

  QPixmap pixmap;
  pixmap.loadFromData(data);
  if (!pixmap.isNull())
    return pixmap;

  QFileIconProvider provider;
  QString path = mPatch.repo().workdir().filePath(mPatch.name());
  QIcon icon = provider.icon(QFileInfo(path));
  return icon.pixmap(windowHandle(), QSize(64, 64));
}

QVBoxLayout *Images::imageLayout(const QPixmap pixmap, int size) {
  QString arg = locale().formattedDataSize(size);
  QLabel *label = new QLabel(tr("<b>Size:</b> %1").arg(arg));
  label->setAlignment(Qt::AlignCenter);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(label);
  layout->addStretch();
  layout->addWidget(new _Images::Image(pixmap, this));
  layout->addStretch();

  return layout;
}
