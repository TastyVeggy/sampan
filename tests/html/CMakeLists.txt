add_executable(test_lexer test_lexer.cpp)

target_link_libraries(test_lexer
    PRIVATE
        warnings
        sampan_html
        GTest::gtest_main
)

add_test(NAME html_lexer_test COMMAND test_lexer)
