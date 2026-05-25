add_executable(test_core
    test_arena.cpp
    test_diagnostic.cpp
    test_log.cpp)

target_link_libraries(test_core PRIVATE sampan_core sampan_warnings sampan_sanitizers GTest::gtest_main)

add_test(NAME core COMMAND test_core)
