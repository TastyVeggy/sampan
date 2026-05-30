add_executable(test_analyze test_analyzer.cpp)

target_link_libraries(test_analyze PRIVATE sampan_analyze sampan_parse sampan_warnings sampan_sanitizers GTest::gtest_main)
target_compile_definitions(test_analyze PRIVATE SAMPAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_test(NAME analyze COMMAND test_analyze)
