add_subdirectory(core)
add_subdirectory(lex)
add_subdirectory(ast)
add_subdirectory(node)
add_subdirectory(parse)
add_subdirectory(analyze)

add_executable(sampan_bin main.cpp)
target_link_libraries(sampan_bin PRIVATE sampan_analyze sampan_core sampan_node sampan_lex sampan_parse sampan_warnings sampan_sanitizers)
target_compile_definitions(sampan_bin PRIVATE SAMPAN_GIT_COMMIT="${SAMPAN_GIT_COMMIT}")
set_target_properties(sampan_bin PROPERTIES OUTPUT_NAME sampan)

install(TARGETS sampan_bin RUNTIME DESTINATION bin)
