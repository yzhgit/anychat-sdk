find_package(SQLite3 REQUIRED)

add_library(anychat_core STATIC
    src/client.cpp
    src/auth_manager.cpp
    src/connection_manager.cpp
    src/notification_manager.cpp
    src/outbound_queue.cpp
    src/sync_engine.cpp
    src/message_manager.cpp
    src/conversation_manager.cpp
    src/friend_manager.cpp
    src/group_manager.cpp
    src/file_manager.cpp
    src/user_manager.cpp
    src/rtc_manager.cpp
    src/db/database.cpp
    src/db/migrations.cpp
    src/network/http_client.cpp
    src/network/websocket_client.cpp
)

target_include_directories(anychat_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(anychat_core PUBLIC cxx_std_17)

# ── Dependencies ──────────────────────────────────────────────────────────────

target_link_libraries(anychat_core
    PUBLIC  nlohmann_json::nlohmann_json
    PRIVATE CURL::libcurl
            SQLite::SQLite3
)

# libwebsockets: handle both Libwebsockets and libwebsockets config names
if(TARGET websockets)
    target_link_libraries(anychat_core PRIVATE websockets)
elseif(TARGET websockets_shared)
    target_link_libraries(anychat_core PRIVATE websockets_shared)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LWS REQUIRED libwebsockets)
        target_include_directories(anychat_core PRIVATE ${LWS_INCLUDE_DIRS})
        target_link_libraries(anychat_core PRIVATE ${LWS_LIBRARIES})
    endif()
endif()
