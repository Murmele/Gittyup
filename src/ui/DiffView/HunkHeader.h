#ifndef HUNKHEADER_H
#define HUNKHEADER_H

#include <QWidget>

class HunkHeader : public QWidget {
public:
  HunkHeader(const QString &name, bool submodule, QWidget *parent = nullptr);
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

#endif // HUNKHEADER_H
