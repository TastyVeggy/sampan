add_library(sampan_node STATIC dump.cpp)

target_include_directories(sampan_node PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_node
  PRIVATE sampan_warnings sampan_sanitizers
  PUBLIC sampan_core)
