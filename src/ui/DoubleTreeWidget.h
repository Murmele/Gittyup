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

#include "DiffTreeModel.h"
#include "DetailView.h"
#include "git/Index.h"
#include <QModelIndexList>
#include "conf/Settings.h"

class QTreeView;
class TreeView;
class BlameEditor;
class StatePushButton;
class DiffView;
class QLabel;

// button in treeview:
// https://stackoverflow.com/questions/40716138/how-to-add-a-button-to-a-qtreeview-row

/*!
 * \brief The DoubleTreeWidget class
 * TreeView like in GitKraken
 */
class DoubleTreeWidget : public ContentWidget {
  Q_OBJECT

public:
  DoubleTreeWidget(const git::Repository &repo, QWidget *parent = nullptr);
  QModelIndex selectedIndex() const override;
  QList<QModelIndex> selectedIndices() const;
  QString selectedFile() const override;

  void setDiff(const git::Diff &diff, const QString &file = QString(),
               const QString &pathspec = QString()) override;

  void cancelBackgroundTasks() override;

  void find() override;
  void findNext() override;
  void findPrevious() override;

  uint32_t setDiffCounter() { return mSetDiffCounter; }

private slots:
  void collapseCountChanged(int count);
  static void showFileContextMenu(const QPoint &pos, RepoView *view,
                                  QTreeView *tree, bool staged);
  static void openExternalDiffTool(const QModelIndex& index, RepoView *view, bool staged);

private:
  enum View {
    Blame,
    Diff,
  };

  void treeModelStateChanged(const QModelIndex &index, int checkState);
  void storeSelection();
  void loadSelection();
  void filesSelected(const QModelIndexList &indexes);
  void loadEditorContent(const QModelIndexList &indexes);
  void toggleCollapseStagedFiles();
  void toggleCollapseUnstagedFiles();
  QAction *setupAppearanceAction(const char *name, Setting::Id id,
                                 bool defaultValue = false);

  DiffTreeModel *mDiffTreeModel{nullptr};
  TreeView *stagedFiles{nullptr};
  TreeView *unstagedFiles{nullptr};
  StatePushButton *collapseButtonStagedFiles{nullptr};
  StatePushButton *collapseButtonUnstagedFiles{nullptr};
  QLabel *mUnstagedCommitedFiles{nullptr};

  struct SelectedFile {
    QString filename;
    bool stagedModel;
  };

  // Determines which file is selected.
  // Is used to restore the selection after a new diff is set
  struct SelectedFile mSelectedFile {
    "", false
  };
  /*!
   * needed to set the visibility. When the diff is a commit, no need for
   * a second TreeView. So the staged one gets hidden.
   */
  QWidget *mStagedWidget{nullptr};
  /*!
   * Shows file content
   */
  BlameEditor *mEditor{nullptr};
  /*!
   * Shows the diff of a file
   */
  DiffView *mDiffView{nullptr};

  /*!
   * Shows BlameEditor or DiffView
   */
  QStackedWidget *mFileView{nullptr};
  bool mIgnoreSelectionChange{false};

  git::Diff mDiff;

  int fileCountExpansionThreshold{100};

  uint32_t mSetDiffCounter{0};

  friend class TestTreeView;
};
#endif // DOUBLETREEWIDGET_H
