cmake_minimum_required(VERSION 3.20)
project(AnyChatSDK VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(RUNTIME_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIR})

# Use ccache when available to speed up builds.
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  message(STATUS "using ccache")
else()
  message(STATUS "ccache not found")
endif()

# ── CPM Package Manager ──────────────────────────────────────────────────────
include(cmake/CPM.cmake)

# ── Third-party dependencies ─────────────────────────────────────────────────

# nlohmann/json (header-only)
CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  VERSION 3.11.3
  OPTIONS "JSON_BuildTests OFF"
)

# libcurl — use system library on Apple/Android toolchains, find via CMake elsewhere
if(APPLE OR ANDROID)
  find_package(CURL REQUIRED)
else()
  find_package(CURL QUIET)
  if(NOT CURL_FOUND)
    message(FATAL_ERROR
      "[AnyChat] libcurl not found.\n"
      "  Ubuntu/Debian : sudo apt install libcurl4-openssl-dev\n"
      "  macOS         : brew install curl\n"
      "  Windows       : vcpkg install curl"
    )
  endif()
endif()

# libwebsockets — find system library; provide install hint on failure
find_package(Libwebsockets QUIET)
if(NOT Libwebsockets_FOUND)
  # Some distros install the CMake config under a different name
  find_package(libwebsockets QUIET)
endif()
if(NOT Libwebsockets_FOUND AND NOT libwebsockets_FOUND)
  message(FATAL_ERROR
    "[AnyChat] libwebsockets not found.\n"
    "  Ubuntu/Debian : sudo apt install libwebsockets-dev\n"
    "  macOS         : brew install libwebsockets\n"
    "  Cross-compile : set -DCMAKE_PREFIX_PATH to your sysroot"
  )
endif()

# ── Build options ─────────────────────────────────────────────────────────────
option(BUILD_TESTS            "Build unit tests"                    ON)
option(BUILD_ANDROID_BINDING  "Build Android JNI binding"           OFF)
option(BUILD_IOS_BINDING      "Build iOS binding"                   OFF)
option(BUILD_WEB_BINDING      "Build WebAssembly binding"           OFF)

# ── Sub-directories ───────────────────────────────────────────────────────────
add_subdirectory(core)

if(BUILD_ANDROID_BINDING)
  add_subdirectory(bindings/android)
endif()

if(BUILD_IOS_BINDING)
  add_subdirectory(bindings/ios)
endif()

if(BUILD_WEB_BINDING)
  add_subdirectory(bindings/web)
endif()

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(core/tests)
endif()
