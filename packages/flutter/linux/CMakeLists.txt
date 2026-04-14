# Linux platform build configuration for anychat_flutter
cmake_minimum_required(VERSION 3.10)

project(anychat_flutter_linux LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(PLUGIN_DIR "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
get_filename_component(ANYCHAT_SDK_ROOT "${PLUGIN_DIR}/../../.." ABSOLUTE)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${ANYCHAT_SDK_ROOT}" anychat_sdk_build)

add_library(anychat_flutter_plugin SHARED
    plugin.cpp
    "${ANYCHAT_SDK_ROOT}/core/src/c_api/utils_c.cpp"
)

target_link_libraries(anychat_flutter_plugin
    PUBLIC -Wl,--whole-archive anychat -Wl,--no-whole-archive
)

set_target_properties(anychat_flutter_plugin PROPERTIES
    PUBLIC_HEADER "${ANYCHAT_SDK_ROOT}/core/include/anychat/anychat.h"
    LINK_FLAGS "-Wl,--no-undefined"
)

install(TARGETS anychat_flutter_plugin
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/anychat
)
