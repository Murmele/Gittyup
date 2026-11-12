#include "RefVisibilityPanel.h"
#include "conf/RefVisibility.h"
#include "git/Reference.h"
#include "ui/MainWindow.h"
#include "ui/RepoView.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

RefVisibilityPanel::RefVisibilityPanel(const git::Repository &repo,
                                       QWidget *parent)
    : QWidget(parent), mRepo(repo) {
  auto *layout = new QVBoxLayout(this);
  auto *description = new QLabel(
      tr("Hide ref names in navigation and commit labels, not commits. "
         "Git operations still have access to all refs. The current branch, "
         "detached HEAD and stash remain visible."),
      this);
  description->setWordWrap(true);
  layout->addWidget(description);

  if (repo) {
    mInherit = new QCheckBox(tr("Use global settings"), this);
    mInherit->setObjectName("inheritRefVisibility");
    layout->addWidget(mInherit);
    connect(mInherit, &QCheckBox::toggled, this, [this] {
      mDirty = true;
      updatePreview();
    });
  }

  mOrigin = new QLabel(this);
  layout->addWidget(mOrigin);
  auto *rulesLabel = new QLabel(
      tr("Hidden refs (one full name or wildcard rule per line):"), this);
  layout->addWidget(rulesLabel);
  mRules = new QPlainTextEdit(this);
  mRules->setObjectName("hiddenRefRules");
  mRules->setPlaceholderText("refs/agents/*\nrefs/heads/archive/*");
  mRules->setMinimumHeight(90);
  rulesLabel->setBuddy(mRules);
  layout->addWidget(mRules);

  auto *syntax = new QLabel(
      tr("Matching is case-sensitive. * matches any text, including /; "
         "? matches one character. Other characters are literal."),
      this);
  syntax->setWordWrap(true);
  layout->addWidget(syntax);

  mSearch = new QLineEdit(this);
  mSearch->setObjectName("refVisibilitySearch");
  mSearch->setPlaceholderText(repo ? tr("Search repository refs")
                                   : tr("Search refs in open repositories"));
  mSearch->setClearButtonEnabled(true);
  layout->addWidget(mSearch);
  mRefs = new QListWidget(this);
  mRefs->setObjectName("refVisibilityList");
  mRefs->setMinimumHeight(140);
  layout->addWidget(mRefs);
  auto *hint = new QLabel(
      tr("Checked refs are hidden. To reveal a wildcard-matched ref, edit "
         "the matching rule above."),
      this);
  hint->setWordWrap(true);
  layout->addWidget(hint);

  mSummary = new QLabel(this);
  mSummary->setWordWrap(true);
  layout->addWidget(mSummary);
  auto *buttons = new QHBoxLayout;
  mShowAll = new QPushButton(tr("Show All"), this);
  mShowAll->setObjectName("showAllRefs");
  buttons->addWidget(mShowAll);
  auto *resetButton = new QPushButton(
      repo ? tr("Inherit Global Settings") : tr("Reset Defaults"), this);
  resetButton->setObjectName("resetRefVisibility");
  buttons->addWidget(resetButton);
  buttons->addStretch();
  mApply = new QPushButton(tr("Apply"), this);
  mApply->setObjectName("applyRefVisibility");
  buttons->addWidget(mApply);
  layout->addLayout(buttons);

  connect(mRules, &QPlainTextEdit::textChanged, this, [this] {
    mDirty = true;
    updatePreview();
  });
  connect(mSearch, &QLineEdit::textChanged, this,
          &RefVisibilityPanel::updatePreview);
  connect(mRefs, &QListWidget::itemChanged, this,
          [this](QListWidgetItem *item) {
            QStringList patterns = editedPatterns();
            patterns.removeAll(item->text());
            if (item->checkState() == Qt::Checked)
              patterns.append(item->text());
            mRules->setPlainText(patterns.join('\n'));
          });
  connect(mShowAll, &QPushButton::clicked, this, [this] {
    if (mInherit)
      mInherit->setChecked(false);
    mRules->clear();
    mDirty = true;
    updatePreview();
  });
  connect(mApply, &QPushButton::clicked, this, &RefVisibilityPanel::apply);
  connect(resetButton, &QPushButton::clicked, this, &RefVisibilityPanel::reset);
  connect(RefVisibility::instance(), &RefVisibility::changed, this, [this] {
    if (!mDirty)
      load();
  });
  load();
}

QStringList RefVisibilityPanel::editedPatterns() const {
  QStringList patterns;
  for (const QString &line : mRules->toPlainText().split('\n')) {
    const QString pattern = line.trimmed();
    if (!pattern.isEmpty() && !patterns.contains(pattern))
      patterns.append(pattern);
  }
  return patterns;
}

void RefVisibilityPanel::load() {
  if (mInherit) {
    QSignalBlocker blocker(mInherit);
    mInherit->setChecked(!RefVisibility::hasOverride(mRepo));
  }
  {
    QSignalBlocker blocker(mRules);
    mRules->setPlainText(RefVisibility::patterns(mRepo).join('\n'));
  }
  mDirty = false;
  updatePreview();
}

void RefVisibilityPanel::updatePreview() {
  const bool inherit = mInherit && mInherit->isChecked();
  const QStringList patterns =
      inherit ? RefVisibility::patterns() : editedPatterns();
  RefVisibility::Rules rules(patterns);
  mRules->setEnabled(!inherit);
  mRules->setVisible(!inherit);
  mOrigin->setText(
      inherit ? tr("Source: global settings\n%1").arg(patterns.join('\n'))
      : mRepo
          ? tr("Source: repository override")
          : tr("Source: global settings (built-in default: refs/agents/*)"));
  const QString error = RefVisibility::validationError(patterns);
  mApply->setEnabled(mDirty && error.isEmpty());

  QList<git::Reference> refs;
  if (mRepo) {
    refs = mRepo.refs();
  } else {
    for (MainWindow *window : MainWindow::windows()) {
      for (int i = 0; i < window->count(); ++i)
        refs.append(window->view(i)->repo().refs());
    }
  }

  QMap<QString, bool> hidden;
  QMap<QString, bool> protectedRefs;
  for (const git::Reference &ref : refs) {
    const QString name = ref.qualifiedName();
    hidden[name] = hidden.value(name) || rules.hides(ref);
    const bool protectedRef =
        ref.isHead() || ref.isDetachedHead() || ref.isStash();
    protectedRefs[name] = protectedRefs.value(name, true) && protectedRef;
  }

  QSignalBlocker blocker(mRefs);
  mRefs->clear();
  int count = 0;
  for (auto it = hidden.cbegin(); it != hidden.cend(); ++it) {
    auto *item = new QListWidgetItem(it.key(), mRefs);
    item->setCheckState(it.value() ? Qt::Checked : Qt::Unchecked);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    bool wildcard = false;
    for (const QString &pattern : patterns) {
      if ((pattern.contains('*') || pattern.contains('?')) &&
          RefVisibility::Rules({pattern}).hides(it.key())) {
        wildcard = true;
        item->setToolTip(tr("Matched by %1").arg(pattern));
        break;
      }
    }
    if (inherit || wildcard || protectedRefs.value(it.key()))
      item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    if (protectedRefs.value(it.key()))
      item->setToolTip(tr("Current checkout or stash: always visible."));
    item->setHidden(!it.key().contains(mSearch->text(), Qt::CaseInsensitive));
    if (it.value())
      ++count;
  }
  mSummary->setText(
      error.isEmpty() ? tr("Preview: %1 of %2 refs hidden. Changes take effect "
                           "when you click Apply.")
                            .arg(count)
                            .arg(hidden.size())
                      : error);
}

void RefVisibilityPanel::apply() {
  const git::Result result = mInherit && mInherit->isChecked()
                                 ? RefVisibility::reset(mRepo)
                                 : RefVisibility::save(editedPatterns(), mRepo);
  if (!result) {
    reportError(
        result.errorString(tr("Unable to save ref visibility settings.")));
    return;
  }
  load();
}

void RefVisibilityPanel::reset() {
  const git::Result result = RefVisibility::reset(mRepo);
  if (!result) {
    reportError(
        result.errorString(tr("Unable to reset ref visibility settings.")));
    return;
  }
  load();
}

void RefVisibilityPanel::reportError(const QString &error) {
  QMessageBox::critical(this, tr("Ref Visibility"), error);
}
