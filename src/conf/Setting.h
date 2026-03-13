#ifndef SETTING_H
#define SETTING_H

#include <QObject>
#include <QMap>

// AI Commit Message Generation Constants
namespace AiCommitConstants {

inline constexpr bool enabled = true;
inline constexpr const char *DefaultServiceUrl = "https://api.mistral.ai/v1";
inline constexpr const char *DefaultModel = "mistral-tiny";
inline constexpr double DefaultTemperature = 0.2;

inline constexpr const char *DefaultSystemMessage =
    "You are a helpful assistant that generates concise git commit messages "
    "following conventional commits format. "
    "Start directly with the commit message without any prefixes like 'Commit "
    "Message:', 'Git', or markdown code blocks. "
    "Follow the format: type(scope): subject\n\nbody";

inline constexpr const char *DefaultPromptMessage =
    "Generate a concise, professional git commit message based on the "
    "following changes:\n\n"
    "Changed files:\n"
    "{{CHANGED_FILES}}\n"
    "Additions:\n"
    "{{ADDITIONS}}\n"
    "Deletions:\n"
    "{{DELETIONS}}\n"
    "File changes:\n"
    "{{FILE_CHANGES}}\n"
    "Total changes: +{{TOTAL_ADDITIONS}} lines, -{{TOTAL_DELETIONS}} "
    "lines\n\n"
    "Follow git commit message conventions:\n"
    "- First line: short summary (50-72 chars max)\n"
    "- Body: detailed explanation (72 chars per line max)\n"
    "- Use imperative mood (e.g., 'Fix bug' not 'Fixed bug')\n"
    "- Be concise but descriptive\n"
    "- Start directly with the commit message (no prefixes like 'Commit Message:' nor markdown in the title)\n"
    "- Follow format: type(scope): subject\\n\\nbody\n"
    "- Consider the commit message idea if available\n"
    "\n"
    "Generate the commit message:";

} // namespace AiCommitConstants

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
    AiServiceUrl,
    EnableAiCommitMessages,
    AiCommitModel,
    AiCommitTemperature,
    AiCommitSystemMessage,
    AiCommitPromptMessage,
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
