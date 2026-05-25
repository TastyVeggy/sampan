add_library(sampan_core STATIC arena.cpp diagnostic.cpp log.cpp)

target_include_directories(sampan_core PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(sampan_core PRIVATE sampan_warnings sampan_sanitizers)
