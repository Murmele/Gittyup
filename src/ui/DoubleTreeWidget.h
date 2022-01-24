//
//          Copyright (c) 2020
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//
// Author: Martin Marmsoler
//

#ifndef DOUBLETREEWIDGET_H
#define DOUBLETREEWIDGET_H

#include "DetailView.h" // ContentWidget

class QTreeView;
class DiffTreeModel;
class TreeView;
class BlameEditor;
class StatePushButton;
class DiffView;
class QLabel;

// button in treeview: https://stackoverflow.com/questions/40716138/how-to-add-a-button-to-a-qtreeview-row

/*!
 * \brief The DoubleTreeWidget class
 * TreeView like in GitKraken
 */
class DoubleTreeWidget : public ContentWidget
{
  Q_OBJECT
  
public:
  DoubleTreeWidget(const git::Repository &repo, QWidget *parent = nullptr);
  QModelIndex selectedIndex() const override;
  QString selectedFile() const override;

  void setDiff(
    const git::Diff &diff,
    const QString &file = QString(),
    const QString &pathspec = QString()) override;

  void cancelBackgroundTasks() override;

  void find() override;
  void findNext() override;
  void findPrevious() override;

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
  void collapseCountChanged(int count);

private:
  enum View {
      Blame,
      Diff,
  };

  void treeModelStateChanged(const QModelIndex& index, int checkState);
  void storeSelection();
  void loadSelection();
  void fileSelected(const QModelIndex &index);
  void loadEditorContent(const QModelIndex &index);
  void toggleCollapseStagedFiles();
  void toggleCollapseUnstagedFiles();

  DiffTreeModel* mDiffTreeModel{nullptr};
  TreeView* stagedFiles{nullptr};
  TreeView* unstagedFiles{nullptr};
  StatePushButton* collapseButtonStagedFiles{nullptr};
  StatePushButton* collapseButtonUnstagedFiles{nullptr};
  QLabel* mUnstagedCommitedFiles{nullptr};

  struct SelectedFile {
    QString filename;
    bool stagedModel;
  };

  // Determines which file is selected.
  // Is used to restore the selection after a new diff is set
  struct SelectedFile mSelectedFile{"", false};
  /*!
   * needed to set the visibility. When the diff is a commit, no need for
   * a second TreeView. So the staged one gets hidden.
   */
  QWidget* mStagedWidget{nullptr};
  /*!
   * Shows file content
   */
  BlameEditor *mEditor{nullptr};
  /*!
   * Shows the diff of a file
   */
  DiffView* mDiffView{nullptr};

  /*!
   * Shows BlameEditor or DiffView
   */
  QStackedWidget* mFileView{nullptr};
  bool mIgnoreSelectionChange{false};

  git::Diff mDiff;
};
#endif // DOUBLETREEWIDGET_H
