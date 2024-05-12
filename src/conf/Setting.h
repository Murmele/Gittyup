#ifndef SETTING_H
#define SETTING_H

#include <QObject>
#include <QMap>

template <class T> class SettingsTempl {
public:
  template <typename TId> static QString key(const TId id) {
    return keys<TId>().value(id);
  }

private:
  template <typename TId> static QMap<TId, QString> keys() {
    static QMap<TId, QString> keys;
    if (keys.isEmpty()) {
      T::initialize(keys);
    }
    return keys;
  }
};

class Setting : public SettingsTempl<Setting> {
  Q_GADGET

public:
  enum class Id {
    FetchAutomatically,
    AutomaticFetchPeriodInMinutes,
    PushAfterEachCommit,
    UpdateSubmodulesAfterPullAndClone,
    PruneAfterFetch,
    FontFamily,
    FontSize,
    UseTabsForIndent,
    IndentWidth,
    TabWidth,
    ShowHeatmapInBlameMargin,
    ShowWhitespaceInEditor,
    ColorTheme,
    ShowFullRepoPath,
    HideLogAutomatically,
    OpenSubmodulesInTabs,
    OpenAllReposInTabs,
    HideMenuBar,
    ShowAvatars,
    ShowMaximized,
    AutoCollapseAddedFiles,
    AutoCollapseDeletedFiles,
    FilemanagerCommand,
    TerminalCommand,
    TerminalName,
    TerminalPath,
    DontTranslate,
    AllowSingleInstanceOnly,
    CheckForUpdatesAutomatically,
    InstallUpdatesAutomatically,
    SkippedUpdates,
    SshConfigFilePath,
    SshKeyFilePath,
    CommitMergeImmediately,
    ShowCommitsInCompactMode,
    ShowCommitsAuthor,
    ShowCommitsDate,
    ShowCommitsId,
    ShowChangedFilesAsList,
    ShowChangedFilesMultiColumn, // For the list only
    ShowChangedFilesInSingleView,
    HideUntracked,
    Language,
  };
  Q_ENUM(Id)

  static void initialize(QMap<Id, QString> &keys);

private:
  Setting() {}
};

class Prompt : public SettingsTempl<Prompt> {
  Q_GADGET

public:
  enum class Kind { Stash, Merge, Revert, CherryPick, Directories, LargeFiles };
  Q_ENUM(Kind)

  static void initialize(QMap<Kind, QString> &keys);
};

#endif
