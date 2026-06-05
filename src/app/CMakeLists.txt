find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

add_library(sampan_app STATIC application.cpp)

target_include_directories(sampan_app PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_app
  PRIVATE sampan_analyze sampan_parse sampan_warnings sampan_sanitizers
  PUBLIC sampan_layout Qt6::Core Qt6::Gui Qt6::Widgets)
