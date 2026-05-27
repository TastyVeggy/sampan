add_library(sampan_ast STATIC dump.cpp)

target_include_directories(sampan_ast PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_ast
  PRIVATE sampan_warnings sampan_sanitizers
  PUBLIC sampan_core)
