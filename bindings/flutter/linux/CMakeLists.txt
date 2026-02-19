# Linux platform build configuration for anychat_flutter
cmake_minimum_required(VERSION 3.10)

project(anychat_flutter_linux LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANYCHAT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# Include the core build
add_subdirectory("${ANYCHAT_SDK_ROOT}/core" anychat_core_build)

# Create the Flutter plugin shared library
add_library(anychat_flutter_plugin SHARED
    # Empty; we're just re-exporting anychat_c
)

target_link_libraries(anychat_flutter_plugin
    PUBLIC anychat_c
)

set_target_properties(anychat_flutter_plugin PROPERTIES
    PUBLIC_HEADER "${ANYCHAT_SDK_ROOT}/core/include/anychat_c/anychat_c.h"
)

install(TARGETS anychat_flutter_plugin LIBRARY DESTINATION lib)
