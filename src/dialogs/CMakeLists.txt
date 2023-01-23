add_library(
  dialogs
  AboutDialog.cpp
  AccountDialog.cpp
  AddRemoteDialog.cpp
  AmendDialog.cpp
  BranchDelegate.cpp
  BranchTableModel.cpp
  CheckoutDialog.cpp
  CloneDialog.cpp
  CommitDialog.cpp
  ConfigDialog.cpp
  DeleteBranchDialog.cpp
  DeleteTagDialog.cpp
  DiffPanel.cpp
  ExternalToolsDialog.cpp
  ExternalToolsModel.cpp
  HotkeysPanel.cpp
  IconLabel.cpp
  MergeDialog.cpp
  NewBranchDialog.cpp
  PluginsPanel.cpp
  PullRequestDialog.cpp
  RebaseConflictDialog.cpp
  RemoteDialog.cpp
  RemoteTableModel.cpp
  SettingsDialog.cpp
  StartDialog.cpp
  SubmoduleDelegate.cpp
  SubmoduleTableModel.cpp
  TagDialog.cpp
  ThemeDialog.cpp
  UpdateSubmodulesDialog.cpp)

target_compile_definitions(
  dialogs PRIVATE GITTYUP_BUILD_DATE="${GITTYUP_BUILD_DATE}"
                  GITTYUP_BUILD_REVISION="${GITTYUP_BUILD_REVISION}")

target_include_directories(dialogs PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
  dialogs
  cli
  conf
  cred
  host
  index
  Qt5::Widgets)

set_target_properties(dialogs PROPERTIES AUTOMOC ON)
