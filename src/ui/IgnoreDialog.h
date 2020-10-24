#ifndef IGNOREDIALOG_H
#define IGNOREDIALOG_H

#include <QDialog>

class QDialogButtonBox;
class QTextEdit;

class IgnoreDialog : public QDialog
{
public:
  IgnoreDialog(QString &ignore, QWidget* parent = nullptr);
private:
  void applyIgnore();

  QDialogButtonBox* mButtonBox{nullptr};
  QTextEdit* mIgnore{nullptr};

  QString& mIgnoreText;
};

#endif // IGNOREDIALOG_H
