add_library(sampan_analyze STATIC
  analyzer.cpp
  builtins.cpp
  child_rules.cpp
  property_rules.cpp)

target_include_directories(sampan_analyze PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_analyze
  PRIVATE sampan_warnings sampan_sanitizers
  PUBLIC sampan_ast sampan_core sampan_node)
