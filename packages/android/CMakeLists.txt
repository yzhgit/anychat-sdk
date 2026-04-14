cmake_minimum_required(VERSION 3.22.1)

project(anychat_android)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the SDK root
set(ANYCHAT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp
    ${ANYCHAT_SDK_ROOT}/core/include
)

# Build the full SDK so the anychat target gets its thirdparty deps.
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${ANYCHAT_SDK_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/sdk)

# JNI bridge sources
set(JNI_SOURCES
    src/main/cpp/jni_bridge.cpp
    src/main/cpp/jni_helpers.cpp
    src/main/cpp/jni_auth.cpp
    src/main/cpp/jni_message.cpp
    src/main/cpp/jni_conversation.cpp
    src/main/cpp/jni_friend.cpp
    src/main/cpp/jni_group.cpp
)

# Create the shared library
add_library(anychat_jni SHARED ${JNI_SOURCES})

# Link against the core library and Android log
target_link_libraries(anychat_jni
    anychat
    android
    log
)

# Set output directory
set_target_properties(anychat_jni PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}"
)
