add_library(sampan_layout STATIC dump.cpp extent.cpp layout.cpp text_layout.cpp)

target_include_directories(sampan_layout PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_layout
  PRIVATE sampan_warnings sampan_sanitizers
  PUBLIC sampan_node)
