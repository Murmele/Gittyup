#include "Setting.h"

void Setting::initialize(QMap<Id, QString> &keys) {
  keys[Id::FetchAutomatically] = "global/autofetch/enable";
  keys[Id::AutomaticFetchPeriodInMinutes] = "global/autofetch/minutes";
  keys[Id::PushAfterEachCommit] = "global/autopush/enable";
  keys[Id::UpdateSubmodulesAfterPullAndClone] = "global/autoupdate/enable";
  keys[Id::PruneAfterFetch] = "global/autoprune/enable";
  keys[Id::FontFamily] = "editor/font/family";
  keys[Id::FontSize] = "editor/font/size";
  keys[Id::UseTabsForIndent] = "editor/indent/tabs";
  keys[Id::IndentWidth] = "editor/indent/width";
  keys[Id::TabWidth] = "editor/indent/tabwidth";
  keys[Id::ShowHeatmapInBlameMargin] = "editor/blame/heatmap";
  keys[Id::ShowWhitespaceInEditor] = "editor/view/showWhitespace";
  keys[Id::ColorTheme] = "window/theme";
  keys[Id::ShowFullRepoPath] = "window/path/full";
  keys[Id::HideLogAutomatically] = "window/log/hide";
  keys[Id::OpenSubmodulesInTabs] = "window/tabs/submodule";
  keys[Id::OpenAllReposInTabs] = "window/tabs/repository";
  keys[Id::HideMenuBar] = "window/view/menuBarHidden";
  keys[Id::ShowAvatars] = "window/view/avatarsVisible";
  keys[Id::ShowMaximized] = "window/view/showMaximized";
  keys[Id::AutoCollapseAddedFiles] = "collapse/added";
  keys[Id::AutoCollapseDeletedFiles] = "collapse/deleted";
  keys[Id::FilemanagerCommand] = "filemanager/command";
  keys[Id::TerminalCommand] = "terminal/command";
  keys[Id::TerminalName] = "terminal/name";
  keys[Id::TerminalPath] = "terminal/path";
  keys[Id::DontTranslate] = "translation/disable";
  keys[Id::Language] = "translation/language";
  keys[Id::AllowSingleInstanceOnly] = "singleInstance";
  keys[Id::CheckForUpdatesAutomatically] = "update/check";
  keys[Id::InstallUpdatesAutomatically] = "update/download";
  keys[Id::SkippedUpdates] = "update/skip";
  keys[Id::SshConfigFilePath] = "ssh/configFilePath";
  keys[Id::SshKeyFilePath] = "ssh/keyFilePath";
  keys[Id::CommitMergeImmediately] = "merge/commit";
  keys[Id::ShowCommitsInCompactMode] = "commit/compact";
  keys[Id::ShowCommitsAuthor] = "commit/author";
  keys[Id::ShowCommitsDate] = "commit/date";
  keys[Id::ShowCommitsId] = "commit/id";
  keys[Id::ShowChangedFilesAsList] = "doubletreeview/listview";
  keys[Id::ShowChangedFilesInSingleView] = "doubletreeview/single";
  keys[Id::ShowChangedFilesMultiColumn] = "doubletreeview/listviewmulticolumn";
  keys[Id::HideUntracked] = "untracked.hide";
}

void Prompt::initialize(QMap<Kind, QString> &keys) {
  const QString root("window/prompt/");
  keys[Kind::Merge] = root + "merge";
  keys[Kind::Stash] = root + "stash";
  keys[Kind::Revert] = root + "revert";
  keys[Kind::CherryPick] = root + "cherrypick";
  keys[Kind::Directories] = root + "directories";
  keys[Kind::LargeFiles] = root + "largeFiles";
}
