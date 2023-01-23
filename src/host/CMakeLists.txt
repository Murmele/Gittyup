add_library(
  host
  Account.cpp
  Accounts.cpp
  Beanstalk.cpp
  Bitbucket.cpp
  GitHub.cpp
  Gitea.cpp
  GitLab.cpp
  Repository.cpp)

target_link_libraries(host conf cred Qt5::Core Qt5::Gui Qt5::Network)

target_compile_definitions(
  host
  PRIVATE GITHUB_CLIENT_ID="${GITHUB_CLIENT_ID}"
          GITHUB_CLIENT_SECRET="${GITHUB_CLIENT_SECRET}"
          GITEA_CLIENT_ID="${GITEA_CLIENT_ID}"
          GITEA_CLIENT_SECRET="${GITEA_CLIENT_SECRET}")

set_target_properties(host PROPERTIES AUTOMOC ON)
