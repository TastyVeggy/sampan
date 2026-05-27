add_executable(test_parse test_parser.cpp)

target_link_libraries(test_parse PRIVATE sampan_parse sampan_warnings sampan_sanitizers GTest::gtest_main)
target_compile_definitions(test_parse PRIVATE SAMPAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_test(NAME parse COMMAND test_parse)
