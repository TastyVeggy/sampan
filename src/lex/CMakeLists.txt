add_library(sampan_lex STATIC lexer.cpp)

target_include_directories(sampan_lex PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_lex PRIVATE sampan_core sampan_warnings sampan_sanitizers)
