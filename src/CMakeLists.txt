add_subdirectory(html)
# add_subdirectory(css)
# add_subdirectory(layout)
# add_subdirectory(net)
# add_subdirectory(render)

add_executable(sampan_bin main.cpp)

target_link_libraries(sampan_bin
    PRIVATE
        warnings
        sanitizers
        sampan_html
)

set_target_properties(sampan_bin PROPERTIES OUTPUT_NAME sampan)
