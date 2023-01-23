#ifndef FILELABEL_H
#define FILELABEL_H

#include <QWidget>

class FileLabel : public QWidget {
public:
  FileLabel(const QString &name, bool submodule, QWidget *parent = nullptr);
  void setName(const QString &name);
  void setOldName(const QString &oldName);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  QString mName;
  QString mOldName;
};

#endif // FILELABEL_H
