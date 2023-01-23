//
//          Copyright (c) 2018, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "PluginsPanel.h"
#include "plugins/Plugin.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

PluginsPanel::PluginsPanel(const git::Repository &repo, QWidget *parent)
    : QTreeWidget(parent), mRepo(repo) {
  QStringList names = {tr("Name"), tr("Kind"), tr("Description")};

  setIndentation(4);
  setRootIsDecorated(false);
  setColumnCount(names.size());
  setHeaderItem(new QTreeWidgetItem(names));
  setSelectionMode(QAbstractItemView::NoSelection);
  header()->setSectionResizeMode(Name, QHeaderView::ResizeToContents);

  refresh();
}

QSize PluginsPanel::sizeHint() const { return QSize(600, 320); }

void PluginsPanel::refresh() {
  clear();

  QFont bold = font();
  bold.setBold(true);

  foreach (PluginRef plugin, Plugin::plugins(mRepo)) {
    QTreeWidgetItem *root = new QTreeWidgetItem(this, {plugin->name()});
    root->setData(Name, Qt::UserRole, QVariant::fromValue(plugin));
    root->setFont(Name, bold);

    // Report error message.
    if (!plugin->isValid()) {
      QString msg = plugin->errorString();
      QTreeWidgetItem *item = new QTreeWidgetItem(root, {msg});
      item->setFirstColumnSpanned(true);
      continue;
    }

    // Add options button.
    QWidget *widget = new QWidget;
    QPushButton *button = new QPushButton(tr("Options"), widget);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    layout->addWidget(button);
    setItemWidget(root, Description, widget);

    QStringList keys = plugin->optionKeys();
    button->setEnabled(!keys.isEmpty());
    connect(button, &QPushButton::clicked, [this, plugin, keys] {
      QDialog dialog;
      dialog.setWindowTitle(tr("%1 Options").arg(plugin->name()));

      QFormLayout *layout = new QFormLayout(&dialog);
      foreach (const QString &key, keys) {
        QWidget *widget = nullptr;
        QVariant value = plugin->optionValue(key);
        switch (plugin->optionKind(key)) {
          case Plugin::Boolean: {
            QCheckBox *checkBox = new QCheckBox(&dialog);
            checkBox->setChecked(value.toBool());
            connect(checkBox, &QCheckBox::toggled, [plugin, key](bool checked) {
              plugin->setOptionValue(key, checked);
            });

            widget = checkBox;
            break;
          }

          case Plugin::Integer: {
            QSpinBox *spinBox = new QSpinBox(&dialog);
            spinBox->setMaximum(999);
            spinBox->setValue(value.toInt());
            auto signal = QOverload<int>::of(&QSpinBox::valueChanged);
            connect(spinBox, signal, [plugin, key](int value) {
              plugin->setOptionValue(key, value);
            });

            widget = spinBox;
            break;
          }

          case Plugin::String: {
            QLineEdit *field = new QLineEdit(value.toString(), &dialog);
            connect(field, &QLineEdit::textChanged,
                    [plugin, key](const QString &text) {
                      plugin->setOptionValue(key, text);
                    });

            widget = field;
            break;
          }

          case Plugin::List: {
            QComboBox *comboBox = new QComboBox(&dialog);
            foreach (const QString &opt, plugin->optionOpts(key))
              comboBox->addItem(opt);
            comboBox->setCurrentIndex(value.toInt() - 1);

            auto signal = QOverload<int>::of(&QComboBox::currentIndexChanged);
            connect(comboBox, signal, [plugin, key](int index) {
              plugin->setOptionValue(key, index + 1);
            });

            widget = comboBox;
            break;
          }
        }

        layout->addRow(QString("%1:").arg(plugin->optionText(key)), widget);
      }

      auto btns = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
      QDialogButtonBox *buttons = new QDialogButtonBox(btns, &dialog);
      connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
      connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
      layout->addRow(buttons);

      if (dialog.exec())
        refresh();
    });

    foreach (const QString &key, plugin->diagnosticKeys()) {
      QString desc = plugin->diagnosticDescription(key);
      QStringList strings = {plugin->diagnosticName(key), QString(), desc};
      QTreeWidgetItem *item = new QTreeWidgetItem(root, strings);
      auto state = plugin->isEnabled(key) ? Qt::Checked : Qt::Unchecked;
      item->setCheckState(Name, state);
      item->setToolTip(Description, desc);
      item->setData(Name, Qt::UserRole, key);

      QComboBox *cb = new QComboBox(this);
      cb->addItem(tr("Note"), Plugin::Note);
      cb->addItem(tr("Warning"), Plugin::Warning);
      cb->addItem(tr("Error"), Plugin::Error);
      cb->setCurrentIndex(cb->findData(plugin->diagnosticKind(key)));
      setItemWidget(item, Kind, cb);

      auto signal = QOverload<int>::of(&QComboBox::currentIndexChanged);
      connect(cb, signal, [plugin, key, cb](int index) {
        int kind = cb->itemData(index).toInt();
        plugin->setDiagnosticKind(key,
                                  static_cast<Plugin::DiagnosticKind>(kind));
      });
    }
  }

  connect(this, &PluginsPanel::itemChanged, [](QTreeWidgetItem *item) {
    QTreeWidgetItem *parent = item->parent();
    if (!parent)
      return;

    QString key = item->data(Name, Qt::UserRole).toString();
    if (key.isEmpty())
      return;

    PluginRef plugin = parent->data(Name, Qt::UserRole).value<PluginRef>();
    plugin->setEnabled(key, item->checkState(0) == Qt::Checked);
  });

  expandAll();
}
