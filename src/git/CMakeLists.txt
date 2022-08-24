add_library(
  git
  AnnotatedCommit.cpp
  Blame.cpp
  Blob.cpp
  Branch.cpp
  Buffer.cpp
  Command.cpp
  Commit.cpp
  Config.cpp
  Diff.cpp
  Filter.cpp
  FilterList.cpp
  Id.cpp
  Index.cpp
  Object.cpp
  Patch.cpp
  Rebase.cpp
  Reference.cpp
  Remote.cpp
  Repository.cpp
  Result.cpp
  RevWalk.cpp
  Signature.cpp
  Submodule.cpp
  Tag.cpp
  TagRef.cpp
  Tree.cpp)

target_link_libraries(git
  ${LIBGIT2_DEPENDENCY_OBJECTS}
  ${LIBGIT2_SYSTEM_LIBS}
  $<TARGET_OBJECTS:libgit2>
  $<TARGET_OBJECTS:util>
  Qt5::Core
  Qt5::Network
)

# needed also by the plugin target so it must be public
target_include_directories(git PUBLIC
    ${LIBGIT2_INCLUDES}
    )

set_target_properties(git PROPERTIES AUTOMOC ON)
