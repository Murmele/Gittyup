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

# we need both libgit2package and libgit2 otherwise we can't find the header
# files
target_link_libraries(git libgit2package libgit2 gittyupUtil Qt6::Core
                      Qt6::Network)

set_target_properties(git PROPERTIES AUTOMOC ON)
