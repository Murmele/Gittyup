#include "TemplateDialog.h"

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

namespace {
const QString kTemplateFileExtension =
    QStringLiteral(".GittyupCommitMessageTemplate");
}

TemplateDialog::TemplateDialog(QList<TemplateButton::Template> &templates,
                               QWidget *parent)
    : QDialog(parent), mTemplates(templates), mNew(templates) {

  QLabel *lbl;
  QHBoxLayout *hBox;
  QHBoxLayout *hBox2;
  QVBoxLayout *vBox;
  QSpacerItem *spacer;

  // first column
  lbl = new QLabel(tr("Name"));
  mName = new QLineEdit(this);
  hBox = new QHBoxLayout();
  hBox->addWidget(lbl);
  hBox->addWidget(mName);

  lbl = new QLabel(tr("Content"));
  mTemplate = new QTextEdit(this);

  spacer =
      new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
  mAdd = new QPushButton(tr("Add"), this);
  hBox2 = new QHBoxLayout();
  hBox2->addItem(spacer);
  hBox2->addWidget(mAdd);

  vBox = new QVBoxLayout();
  vBox->addLayout(hBox);
  vBox->addWidget(lbl);
  vBox->addWidget(mTemplate);
  vBox->addWidget(new QLabel(tr("use %1 to declare the position of the cursor.")
                                 .arg(TemplateButton::cursorPositionString),
                             this));
  vBox->addWidget(
      new QLabel(tr("use ${files:x} to add all updated file names,\nx (number) "
                    "determines the number of maximum files shown"),
                 this));
  vBox->addLayout(hBox2);

  // second column
  mTemplateList = new QListWidget(this);

  spacer =
      new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
  mRemove = new QPushButton(tr("Remove"));
  hBox = new QHBoxLayout();
  hBox->addItem(spacer);
  hBox->addWidget(mRemove);
  QVBoxLayout *vBox2 = new QVBoxLayout();
  vBox2->addWidget(
      new QLabel(tr("First template will be applied automatically"), this));
  vBox2->addWidget(mTemplateList);
  vBox2->addLayout(hBox);

  // third column
  spacer =
      new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
  mUp = new QPushButton(tr("Up"), this);
  mDown = new QPushButton(tr("Down"), this);
  QSpacerItem *spacer2 =
      new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
  QVBoxLayout *vBox3 = new QVBoxLayout();
  vBox3->addItem(spacer);
  vBox3->addWidget(mUp);
  vBox3->addWidget(mDown);
  vBox3->addItem(spacer2);

  hBox = new QHBoxLayout();
  hBox->addLayout(vBox);
  hBox->addLayout(vBox2);
  hBox->addLayout(vBox3);

  // Import, export, ok, cancel
  auto importButton = new QPushButton(tr("Import"), this);
  auto exportButton = new QPushButton(tr("Export"), this);
  spacer =
      new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
  mButtonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  hBox2 = new QHBoxLayout();
  hBox2->addWidget(importButton);
  hBox2->addWidget(exportButton);
  hBox2->addItem(spacer);
  hBox2->addWidget(mButtonBox);

  vBox = new QVBoxLayout();
  vBox->addLayout(hBox);
  vBox->addLayout(hBox2);

  setLayout(vBox);

  // setting widgets

  for (auto templ : templates) {
    mTemplateList->addItem(templ.name);
  }

  connect(mAdd, &QPushButton::pressed, this, &TemplateDialog::addTemplate);
  connect(mRemove, &QPushButton::pressed, this,
          &TemplateDialog::removeTemplate);
  connect(mUp, &QPushButton::pressed, this, &TemplateDialog::moveTemplateUp);
  connect(mDown, &QPushButton::pressed, this,
          &TemplateDialog::moveTemplateDown);
  connect(mButtonBox, &QDialogButtonBox::accepted, this,
          &TemplateDialog::applyTemplates);
  connect(mButtonBox, &QDialogButtonBox::rejected, this,
          &TemplateDialog::reject);
  connect(mName, &QLineEdit::textChanged, this, &TemplateDialog::checkName);
  connect(mTemplateList, &QListWidget::currentRowChanged, this,
          &TemplateDialog::showTemplate);
  connect(importButton, &QPushButton::pressed, [this] { importTemplates(); });
  connect(exportButton, &QPushButton::pressed, [this] { exportTemplates(); });
}

void TemplateDialog::addTemplate() {
  QString name = mName->text();
  QString value = mTemplate->toPlainText();

  for (int i = 0; i < mNew.count(); i++) {
    if (mNew[i].name == name) {
      // replace value
      mNew[i].value = value;
      return;
    }
  }

  TemplateButton::Template tmpl;
  tmpl.name = name;
  tmpl.value = value;

  mNew.append(tmpl);

  mTemplateList->addItem(tmpl.name);

  checkName(tmpl.name);
}

void TemplateDialog::removeTemplate() {
  QListWidgetItem *itm = mTemplateList->currentItem();
  if (!itm) // no item selected
    return;

  mSupress = true;

  QString name = itm->text();

  for (int i = 0; i < mNew.count(); i++) {
    if (mNew[i].name == name) {
      mNew.takeAt(i);
      QListWidgetItem *itm = mTemplateList->takeItem(i);
      delete itm;
      break;
    }
  }

  mSupress = false;

  if (mNew.count())
    showTemplate(mTemplateList->currentRow());
}

void TemplateDialog::moveTemplateUp() {
  QListWidgetItem *curr = mTemplateList->currentItem();
  if (!curr)
    return;

  QString name = curr->text();

  for (int i = 0; i < mNew.count(); i++) {
    if (mNew[i].name == name) {
      if (i - 1 < 0)
        return;

      mSupress = true;

      TemplateButton::Template tmpl = mNew.takeAt(i);
      mNew.insert(i - 1, tmpl);

      QListWidgetItem *itm = mTemplateList->takeItem(i);
      assert(curr->text() == itm->text());
      mTemplateList->insertItem(i - 1, itm);

      mSupress = false;
      break;
    }
  }
  showTemplate(mTemplateList->currentRow());
}

void TemplateDialog::moveTemplateDown() {
  QListWidgetItem *curr = mTemplateList->currentItem();
  if (!curr)
    return;

  QString name = curr->text();

  for (int i = 0; i < mNew.count(); i++) {
    if (mNew[i].name == name) {
      if (i + 1 >= mNew.count())
        return;

      mSupress = true;

      TemplateButton::Template tmpl = mNew.takeAt(i);
      mNew.insert(i + 1, tmpl);

      QListWidgetItem *itm = mTemplateList->takeItem(i);
      assert(curr->text() == itm->text());
      mTemplateList->insertItem(i + 1, itm);

      mSupress = false;
      break;
    }
  }

  showTemplate(mTemplateList->currentRow());
}

void TemplateDialog::importTemplates(QString filename) {
  if (filename.isEmpty()) {
    filename = QFileDialog::getOpenFileName(
        this, tr("Open File"), "/home",
        tr("Gittyup Templates (*%1)").arg(kTemplateFileExtension));
  }

  mNew.clear();
  mTemplateList->clear();

  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    while (!file.atEnd()) {
      QString line = file.readLine();
      line.remove(line.length() - 1, 1);
      const int index = line.indexOf(QStringLiteral(":"));
      if (index == -1)
        continue;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      const QString name = line.sliced(0, index);
      if (index + 1 >= line.length())
        continue;
      QString value = line.sliced(index + 1);
#else
      const auto list = line.split(QStringLiteral(":"));
      if (list.length() < 2)
        continue;
      const QString name = list.at(0);
      QString value;
      for (int i = 1; i < list.length() - 1; i++)
        value += QStringLiteral("%1:").arg(list.at(i));
      value += list.last();
#endif
      value = value.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
      value = value.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
      TemplateButton::Template t;
      t.name = name;
      t.value = value;
      mNew.append(t);
    }
  }
  if (mNew.count() > 0) {
    for (const auto &t : mNew)
      mTemplateList->addItem(t.name);
    showTemplate(0);
  } else {
    mName->setText(QStringLiteral(""));
    mTemplate->setText(QStringLiteral(""));
  }
}

void TemplateDialog::exportTemplates(QString filename) {
  if (filename.isEmpty()) {
    filename = QFileDialog::getSaveFileName(
        this, tr("Save Templates"),
        QStringLiteral("/home/%1%2")
            .arg("GittyupTemplates", kTemplateFileExtension),
        tr("Gittyup Templates (*%1)").arg(kTemplateFileExtension));
  }

  QString templatesStr;
  for (const auto &tmpl : mNew) {
    QString name = tmpl.name;
    QString value = tmpl.value;
    value = value.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    value = value.replace(QStringLiteral("\t"), QStringLiteral("\\t"));
    templatesStr += QStringLiteral("%1:%2\n").arg(name, value);
  }

  QFile file(filename);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream stream(&file);
    stream << templatesStr;
  }
}

void TemplateDialog::applyTemplates() {
  mTemplates = mNew;
  accept();
}

void TemplateDialog::checkName(QString name) {
  if (!uniqueName(name)) {
    mAdd->setText(tr("Replace"));
    return;
  }

  mAdd->setText(tr("Add"));
}

void TemplateDialog::showTemplate(int idx) {
  if (mSupress)
    return;

  // TODO: called before item is inserted?
  // maybe index is anymore valid
  mName->setText(mNew[idx].name);
  mTemplate->setText(mNew[idx].value);
}

bool TemplateDialog::uniqueName(QString name) {
  for (auto templ : mNew) {
    if (templ.name == name)
      return false;
  }
  return true;
}
