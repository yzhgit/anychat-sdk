#pragma once

#include "types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Client configuration ---- */
typedef struct {
    const char* gateway_url;           /* WebSocket gateway, e.g. "wss://api.anychat.io" */
    const char* api_base_url;          /* HTTP API base, e.g. "https://api.anychat.io/api/v1" */
    const char* device_id;             /* Unique device identifier */
    const char* db_path;               /* SQLite database file path */
    int         connect_timeout_ms;    /* default: 10000 */
    int         max_reconnect_attempts;/* default: 5 */
    int         auto_reconnect;        /* 1 = enabled (default), 0 = disabled */
} AnyChatClientConfig_C;

/* Connection state change callback.
 * state: one of ANYCHAT_STATE_* constants */
typedef void (*AnyChatConnectionStateCallback)(void* userdata, int state);

/* ---- Lifecycle ---- */

/* Create a new client. Returns NULL on failure; call anychat_get_last_error().
 * The caller owns the handle and must destroy it with anychat_client_destroy(). */
ANYCHAT_C_API AnyChatClientHandle anychat_client_create(
    const AnyChatClientConfig_C* config);

/* Destroy the client and release all resources.
 * Must not be called while callbacks are in flight on other threads. */
ANYCHAT_C_API void anychat_client_destroy(AnyChatClientHandle handle);

/* Start the WebSocket connection (non-blocking). */
ANYCHAT_C_API void anychat_client_connect(AnyChatClientHandle handle);

/* Disconnect and stop all background activity (non-blocking). */
ANYCHAT_C_API void anychat_client_disconnect(AnyChatClientHandle handle);

/* Returns the current connection state (ANYCHAT_STATE_*). */
ANYCHAT_C_API int anychat_client_get_connection_state(AnyChatClientHandle handle);

/* Register a callback for connection state changes.
 * Pass NULL to clear a previously registered callback.
 * userdata is passed through unchanged. */
ANYCHAT_C_API void anychat_client_set_connection_callback(
    AnyChatClientHandle             handle,
    void*                           userdata,
    AnyChatConnectionStateCallback  callback);

/* ---- Sub-module accessors ----
 * The returned handles are owned by the client; do NOT destroy them separately. */
ANYCHAT_C_API AnyChatAuthHandle    anychat_client_get_auth(AnyChatClientHandle handle);
ANYCHAT_C_API AnyChatMessageHandle anychat_client_get_message(AnyChatClientHandle handle);
ANYCHAT_C_API AnyChatConvHandle    anychat_client_get_conversation(AnyChatClientHandle handle);
ANYCHAT_C_API AnyChatFriendHandle  anychat_client_get_friend(AnyChatClientHandle handle);
ANYCHAT_C_API AnyChatGroupHandle   anychat_client_get_group(AnyChatClientHandle handle);
ANYCHAT_C_API AnyChatFileHandle    anychat_client_get_file(AnyChatClientHandle handle);
ANYCHAT_C_API AnyChatUserHandle    anychat_client_get_user(AnyChatClientHandle handle);
ANYCHAT_C_API AnyChatRtcHandle     anychat_client_get_rtc(AnyChatClientHandle handle);

#ifdef __cplusplus
}
#endif
