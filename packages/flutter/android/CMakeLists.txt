# Android platform build configuration for anychat_flutter
cmake_minimum_required(VERSION 3.10)

project(anychat_android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANYCHAT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${ANYCHAT_SDK_ROOT}" anychat_sdk_build)

add_library(anychat_android SHARED
    ${ANYCHAT_SDK_ROOT}/core/src/c_api/utils_c.cpp
)

target_link_libraries(anychat_android
    PUBLIC anychat
)

set_target_properties(anychat_android PROPERTIES
    LINK_FLAGS "-Wl,--whole-archive $<TARGET_FILE:anychat> -Wl,--no-whole-archive"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(anychat_android PROPERTIES
        LINK_FLAGS_RELEASE "-s"
    )
endif()
