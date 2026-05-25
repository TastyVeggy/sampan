add_subdirectory(core)

add_executable(sampan_bin main.cpp)
target_link_libraries(sampan_bin PRIVATE sampan_core sampan_warnings sampan_sanitizers)
target_compile_definitions(sampan_bin PRIVATE SAMPAN_GIT_COMMIT="${SAMPAN_GIT_COMMIT}")
set_target_properties(sampan_bin PROPERTIES OUTPUT_NAME sampan)

install(TARGETS sampan_bin RUNTIME DESTINATION bin)
