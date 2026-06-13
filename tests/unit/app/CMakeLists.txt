add_executable(test_app test_document_view.cpp)

target_link_libraries(test_app PRIVATE sampan_analyze sampan_app sampan_parse
    sampan_warnings sampan_sanitizers GTest::gtest)

add_test(NAME app COMMAND test_app)
set_tests_properties(app PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
