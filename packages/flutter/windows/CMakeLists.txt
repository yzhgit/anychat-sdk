# Windows platform build configuration for anychat_flutter
cmake_minimum_required(VERSION 3.14)

project(anychat_flutter_windows LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANYCHAT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${ANYCHAT_SDK_ROOT}" anychat_sdk_build)

add_library(anychat_flutter_plugin SHARED
    ${ANYCHAT_SDK_ROOT}/core/src/c_api/utils_c.cpp
)

target_link_libraries(anychat_flutter_plugin
    PUBLIC anychat
)

set_target_properties(anychat_flutter_plugin PROPERTIES
    PUBLIC_HEADER "${ANYCHAT_SDK_ROOT}/core/include/anychat/anychat.h"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

install(TARGETS anychat_flutter_plugin RUNTIME DESTINATION bin)
