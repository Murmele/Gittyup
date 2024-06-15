#ifndef IGNOREDIALOG_H
#define IGNOREDIALOG_H

#include <QDialog>

class QDialogButtonBox;
class QTextEdit;

class IgnoreDialog : public QDialog {
public:
  IgnoreDialog(const QString &ignore, QWidget *parent = nullptr);
  QString ignoreText() const;

private:
  QDialogButtonBox *mButtonBox{nullptr};
  QTextEdit *mIgnore{nullptr};
};

#endif // IGNOREDIALOG_H
