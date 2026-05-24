add_library(
    sampan_html STATIC
    "${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/html/lexer.hpp"
    lexer.cpp
)

target_link_libraries(sampan_html
    PRIVATE
        warnings
        sanitizers
)

target_include_directories(sampan_html
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

