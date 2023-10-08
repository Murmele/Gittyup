//
//          Copyright (c) 2016, Scientific Toolworks, Inc.
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//

#include "ColumnView.h"
#include "TreeModel.h"
#include "ViewDelegate.h"

#include <QFormLayout>
#include <QItemDelegate>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#define ICON_SIZE 48
#define SCROLL_BAR_WIDTH 18
#else
#define ICON_SIZE 64
#define SCROLL_BAR_WIDTH 0
#endif

namespace {

const QString kNameFmt = "<p style='font-size: large'>%1</p>";
const QString kLabelFmt = "<p style='color: gray; font-weight: bold'>%1</p>";

/*!
 * \brief The PreviewWidget class
 * Widget shown in the next column when file selected instead of a folder
 */
class PreviewWidget : public QFrame {
  Q_OBJECT

public:
  PreviewWidget(ColumnView *parent) : QFrame(parent) {
    mIcon = new QLabel(this);
    mIcon->setAlignment(Qt::AlignHCenter);

    mName = new QLabel(this);
    mName->setAlignment(Qt::AlignHCenter);
    mName->setContentsMargins(4, 4, 4, 4);

    mKind = new QLabel(this);
    mKind->setAlignment(Qt::AlignHCenter);

    mAdded = new QLabel(this);
    mModified = new QLabel(this);

    connect(mAdded, &QLabel::linkActivated, parent, &ColumnView::linkActivated);
    connect(mModified, &QLabel::linkActivated, parent,
            &ColumnView::linkActivated);

    QFormLayout *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
    form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    form->setHorizontalSpacing(4);
    form->setVerticalSpacing(2);
    form->addRow(kLabelFmt.arg(tr("Added")), mAdded);
    form->addRow(kLabelFmt.arg(tr("Modified")), mModified);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->addWidget(mIcon);
    layout->addWidget(mName);
    layout->addWidget(mKind);
    layout->addLayout(form);
  }

  void setFile(const QModelIndex &index, int width) {
    mIndex = index;
    if (!index.isValid() || index.model()->rowCount(index) > 0) {
      hide();
      return;
    }

    QWindow *win = window()->windowHandle();
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    mIcon->setPixmap(
        icon.pixmap(QSize(ICON_SIZE, ICON_SIZE), window()->devicePixelRatio()));

    mName->setText(kNameFmt.arg(index.data(Qt::DisplayRole).toString()));

    QString kind = index.data(TreeModel::KindRole).toString();
    mKind->setVisible(!kind.isEmpty());
    mKind->setText(kLabelFmt.arg(kind));

    mAdded->setText(index.data(TreeModel::AddedRole).toString());
    mModified->setText(index.data(TreeModel::ModifiedRole).toString());

    // QColumnView resizes the preview container
    // to the *size* of the preview widget.
    resize(QSize(width, sizeHint().height()));
    show();
  }

signals:
  void iconDoubleClicked(const QModelIndex &index);

protected:
  void mouseDoubleClickEvent(QMouseEvent *event) override {
    if (mIcon->geometry().contains(event->pos()))
      emit iconDoubleClicked(mIndex);
  }

private:
  QLabel *mIcon;
  QLabel *mName;
  QLabel *mKind;
  QLabel *mAdded;
  QLabel *mModified;
  QModelIndex mIndex;
};

} // namespace

ColumnView::ColumnView(QWidget *parent)
    : QColumnView(parent), mSharedDelegate(new ViewDelegate(this)) {
  PreviewWidget *preview = new PreviewWidget(this);
  connect(preview, &PreviewWidget::iconDoubleClicked, this,
          &ColumnView::doubleClicked);

  setPreviewWidget(preview);
}

void ColumnView::setModel(QAbstractItemModel *model) {
  QColumnView::setModel(model);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &ColumnView::handleSelectionChange);
}

bool ColumnView::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    QWidget *columnViewport = static_cast<QWidget *>(obj);
    QPoint globalPos =
        static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
    QModelIndex index = indexAt(viewport()->mapFromGlobal(globalPos));
    if (!columnViewport->hasFocus() && index.row() < 0) {
      columnViewport->setFocus();
      selectionModel()->clearSelection();
    }
  }

  return false;
}

QAbstractItemView *ColumnView::createColumn(const QModelIndex &index) {
  QAbstractItemView *view = QColumnView::createColumn(index);
  view->setItemDelegate(mSharedDelegate);
  view->viewport()->installEventFilter(this);
  return view;
}

void ColumnView::handleSelectionChange(const QItemSelection &selected,
                                       const QItemSelection &deselected) {
  // FIXME: The argument sent by Qt doesn't contain the whole selection.
  int width = columnWidths().last() - SCROLL_BAR_WIDTH;
  QModelIndexList indexes = selectionModel()->selectedIndexes();
  QModelIndex index = (indexes.size() == 1) ? indexes.first() : QModelIndex();
  static_cast<PreviewWidget *>(previewWidget())->setFile(index, width);
  emit fileSelected(index);

  // Handle deselection.
  if (indexes.isEmpty() && !deselected.indexes().isEmpty()) {
    QModelIndex parent = deselected.indexes().first().parent();
    setCurrentIndex(parent);
    if (!parent.isValid())
      setRootIndex(QModelIndex());
  }
}

#include "ColumnView.moc"
