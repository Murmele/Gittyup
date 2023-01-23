#ifndef IMAGES_H
#define IMAGES_H

#include <QWidget>

#include "git/Patch.h"
#include "git/Blob.h"

class QVBoxLayout;

namespace _Images {
class Image : public QWidget {
public:
  Image(const QPixmap &pixmap, QWidget *parent = nullptr);
  QSize sizeHint() const override;
  bool hasHeightForWidth() const override;
  int heightForWidth(int w) const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QPixmap mPixmap;
};

class Arrow : public QWidget {
public:
  Arrow(QWidget *parent = nullptr);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
};
} // namespace _Images

class Images : public QWidget {
  Q_OBJECT

public:
  Images(const git::Patch patch, bool lfs = false, QWidget *parent = nullptr);

private:
  QPixmap loadPixmap(git::Diff::File type, int &size, bool lfs);
  QVBoxLayout *imageLayout(const QPixmap pixmap, int size);

  git::Patch mPatch;
};

#endif // IMAGES_H
