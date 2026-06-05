add_executable(test_layout test_layout.cpp)

target_link_libraries(test_layout PRIVATE sampan_analyze sampan_layout sampan_parse sampan_warnings sampan_sanitizers GTest::gtest_main)

add_test(NAME layout COMMAND test_layout)
