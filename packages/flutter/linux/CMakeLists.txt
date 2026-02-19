# Linux platform build configuration for anychat_sdk
#
# This CMake file delegates to the actual FFI bindings in ../../bindings/flutter/linux/
# The bindings contain the CMakeLists.txt that builds the native library.

cmake_minimum_required(VERSION 3.10)

project(anychat_sdk_plugin LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the plugin name
set(PLUGIN_NAME "anychat_sdk_plugin")

# Include the actual bindings build
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/flutter/linux
                 ${CMAKE_CURRENT_BINARY_DIR}/anychat_flutter_bindings)

# No additional target needed - the bindings create anychat_flutter_plugin
# which will be loaded by dart:ffi
