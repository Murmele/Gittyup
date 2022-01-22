# Define test macro.
macro(test TEST_NAME)
  add_executable(test_${TEST_NAME} ${TEST_NAME}.cpp)
  target_link_libraries(test_${TEST_NAME} testlib)

  set_target_properties(test_${TEST_NAME} PROPERTIES
    OUTPUT_NAME ${TEST_NAME}
    AUTOMOC ON
  )

  target_include_directories(test_${TEST_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
  )

  add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:test_${TEST_NAME}>)
  add_dependencies(check test_${TEST_NAME})

  if(WIN32)
    STRING(REPLACE ";" "\;" NEWPATH "$ENV{PATH}")
    set_tests_properties(${TEST_NAME} PROPERTIES
      ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}\;${NEWPATH};QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}\\Plugins"
    )
  endif()
  message("PATH: " ${CMAKE_BINARY_DIR})
endmacro()

# Add check target.
add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND}
    --force-new-ctest-process --output-on-failure
  COMMENT "Running tests..."
  USES_TERMINAL
)

enable_testing()

# Add test library.
add_subdirectory(dep/zip)
add_library(testlib Test.cpp)
target_link_libraries(testlib app git ui Qt5::Test zip)
target_include_directories(testlib PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(testlib PRIVATE
  TESTREPOSITORIES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/testRepos"
)


# Add tests.
test(bare_repo)
test(init_repo)
test(merge)
test(external_tools_dialog)
test(config)
test(branches_panel)
test(editor)
test(index)
test(line_endings)
test(log)
test(main_window)
test(new_branch_dialog)
test(sanity)
test(HunkWidget)
test(Diff)
test(EditorLineInfos)
