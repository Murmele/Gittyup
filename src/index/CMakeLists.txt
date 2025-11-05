add_library(
  index
  GenericLexer.cpp
  Index.cpp
  IndexModel.cpp
  Lexer.cpp
  LPegLexer.cpp
  MmapFileReader.cpp
  Query.cpp)

target_link_libraries(index conf git lpeg Qt6::Core Qt6::Concurrent)

set_target_properties(index PROPERTIES AUTOMOC ON)
# if(ENABLE_TESTS) add_executable(lexer_test lexer_test.cpp)
# target_link_libraries(lexer_test index)

add_executable(lexer_test lexer_test.cpp)
target_link_libraries(lexer_test index)

add_executable(index_test index_test.cpp)
target_link_libraries(index_test index Qt6::Widgets)

add_subdirectory(Indexer)
