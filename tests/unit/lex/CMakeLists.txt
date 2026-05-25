add_executable(test_lex test_lexer.cpp)

target_link_libraries(test_lex PRIVATE sampan_lex sampan_warnings sampan_sanitizers GTest::gtest_main)
target_compile_definitions(test_lex PRIVATE SAMPAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

add_test(NAME lex COMMAND test_lex)
