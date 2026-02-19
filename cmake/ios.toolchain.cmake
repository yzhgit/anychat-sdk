# iOS Toolchain for CMake
# Based on https://github.com/leetal/ios-cmake
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=OS64 ..
#
# PLATFORM options:
#   OS64         - iOS Device (arm64)
#   SIMULATOR64  - iOS Simulator (x86_64)
#   SIMULATORARM64 - iOS Simulator on Apple Silicon (arm64)

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_VERSION 13.0)

# Platform selection
if(NOT DEFINED PLATFORM)
    set(PLATFORM "OS64")
endif()

if(PLATFORM STREQUAL "OS64")
    set(CMAKE_OSX_ARCHITECTURES arm64)
    set(CMAKE_OSX_SYSROOT iphoneos)
    set(IOS_PLATFORM_DEVICE TRUE)
elseif(PLATFORM STREQUAL "SIMULATOR64")
    set(CMAKE_OSX_ARCHITECTURES x86_64)
    set(CMAKE_OSX_SYSROOT iphonesimulator)
    set(IOS_PLATFORM_SIMULATOR TRUE)
elseif(PLATFORM STREQUAL "SIMULATORARM64")
    set(CMAKE_OSX_ARCHITECTURES arm64)
    set(CMAKE_OSX_SYSROOT iphonesimulator)
    set(IOS_PLATFORM_SIMULATOR TRUE)
else()
    message(FATAL_ERROR "Invalid PLATFORM: ${PLATFORM}. Must be OS64, SIMULATOR64, or SIMULATORARM64")
endif()

# Deployment target
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum iOS version")
endif()

# Skip compiler checks
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Find developer tools
execute_process(
    COMMAND xcode-select -print-path
    OUTPUT_VARIABLE XCODE_DEVELOPER_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set compilers
set(CMAKE_C_COMPILER ${XCODE_DEVELOPER_DIR}/usr/bin/clang)
set(CMAKE_CXX_COMPILER ${XCODE_DEVELOPER_DIR}/usr/bin/clang++)

# Bitcode support (deprecated in Xcode 14, but keep for compatibility)
if(NOT DEFINED ENABLE_BITCODE)
    set(ENABLE_BITCODE FALSE)
endif()

if(ENABLE_BITCODE)
    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "YES")
else()
    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
endif()

# Only search in iOS SDK
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
