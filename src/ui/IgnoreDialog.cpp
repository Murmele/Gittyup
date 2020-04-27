#include "IgnoreDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>


IgnoreDialog::IgnoreDialog(QString& ignore, QWidget* parent): QDialog(parent), mIgnoreText(ignore)
{
    QLabel* lbl = new QLabel(tr("Ignore Pattern"));
    mIgnore = new QTextEdit(this);
    mIgnore->setText(ignore);

    // TODO: show preview of files which are effected

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->addWidget(lbl);
    vBox->addWidget(mIgnore);
    vBox->addWidget(mButtonBox);

    setLayout(vBox);

    connect(mButtonBox, &QDialogButtonBox::accepted, this, &IgnoreDialog::applyIgnore);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &IgnoreDialog::reject);

}

void IgnoreDialog::applyIgnore()
{
    mIgnoreText = mIgnore->toPlainText();
    accept();
}
