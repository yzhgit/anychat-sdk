cmake_minimum_required(VERSION 3.20)
project(AnyChatWeb)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This CMakeLists.txt is for Emscripten only. Use emcmake cmake ...")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANYCHAT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${ANYCHAT_SDK_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/sdk")

add_executable(anychat_wasm
    src/embind_wrapper.cpp
)

target_link_libraries(anychat_wasm
    PRIVATE anychat
)

set_target_properties(anychat_wasm PROPERTIES
    SUFFIX ".js"
    OUTPUT_NAME "anychat"
)

target_link_options(anychat_wasm PRIVATE
    "SHELL:--bind"
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s EXPORT_NAME='createAnyChatModule'"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s ENVIRONMENT=web,worker"
    "SHELL:-s FILESYSTEM=0"
    "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    "SHELL:--post-js ${CMAKE_CURRENT_SOURCE_DIR}/src/anychat_post.js"
)

file(GLOB TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ts")
foreach(ts_file ${TS_FILES})
    configure_file(${ts_file} ${CMAKE_CURRENT_BINARY_DIR}/src/ COPYONLY)
endforeach()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/package.json
    ${CMAKE_CURRENT_BINARY_DIR}/package.json
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tsconfig.json
    ${CMAKE_CURRENT_BINARY_DIR}/tsconfig.json
    COPYONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/anychat.js
    ${CMAKE_CURRENT_BINARY_DIR}/anychat.wasm
    DESTINATION lib
)
