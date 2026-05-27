add_library(sampan_parse STATIC parser.cpp)

target_include_directories(sampan_parse PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_parse
  PRIVATE sampan_warnings sampan_sanitizers
  PUBLIC sampan_ast sampan_core sampan_lex)
