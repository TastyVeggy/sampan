find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Network Widgets)

add_library(sampan_app STATIC
  application.cpp
  browser_window.cpp
  document_loader.cpp
  document_view.cpp)

qt_add_resources(sampan_app "sampan_internal_pages"
  PREFIX "/sampan"
  BASE "${CMAKE_CURRENT_SOURCE_DIR}/pages"
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/pages/home.yl")

target_include_directories(sampan_app PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(sampan_app
  PRIVATE sampan_analyze sampan_parse sampan_warnings sampan_sanitizers
  PUBLIC sampan_layout Qt6::Core Qt6::Gui Qt6::Network Qt6::Widgets)
