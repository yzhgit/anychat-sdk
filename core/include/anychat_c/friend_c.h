#pragma once

#include "types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Callback types ---- */

typedef void (*AnyChatFriendListCallback)(
    void*                    userdata,
    const AnyChatFriendList_C* list,
    const char*              error);

typedef void (*AnyChatFriendRequestListCallback)(
    void*                            userdata,
    const AnyChatFriendRequestList_C* list,
    const char*                      error);

typedef void (*AnyChatFriendCallback)(
    void*       userdata,
    int         success,
    const char* error);

/* Fired when a new friend request arrives (WebSocket notification). */
typedef void (*AnyChatFriendRequestCallback)(
    void*                        userdata,
    const AnyChatFriendRequest_C* request);

/* Fired when the friend list changes (add/remove). */
typedef void (*AnyChatFriendListChangedCallback)(void* userdata);

/* ---- Friend operations ---- */

ANYCHAT_C_API int anychat_friend_get_list(
    AnyChatFriendHandle        handle,
    void*                      userdata,
    AnyChatFriendListCallback  callback);

ANYCHAT_C_API int anychat_friend_send_request(
    AnyChatFriendHandle   handle,
    const char*           to_user_id,
    const char*           message,
    void*                 userdata,
    AnyChatFriendCallback callback);

/* accept: 1 to accept, 0 to reject */
ANYCHAT_C_API int anychat_friend_handle_request(
    AnyChatFriendHandle   handle,
    int64_t               request_id,
    int                   accept,
    void*                 userdata,
    AnyChatFriendCallback callback);

ANYCHAT_C_API int anychat_friend_get_pending_requests(
    AnyChatFriendHandle             handle,
    void*                           userdata,
    AnyChatFriendRequestListCallback callback);

ANYCHAT_C_API int anychat_friend_delete(
    AnyChatFriendHandle   handle,
    const char*           friend_id,
    void*                 userdata,
    AnyChatFriendCallback callback);

ANYCHAT_C_API int anychat_friend_update_remark(
    AnyChatFriendHandle   handle,
    const char*           friend_id,
    const char*           remark,
    void*                 userdata,
    AnyChatFriendCallback callback);

ANYCHAT_C_API int anychat_friend_add_to_blacklist(
    AnyChatFriendHandle   handle,
    const char*           user_id,
    void*                 userdata,
    AnyChatFriendCallback callback);

ANYCHAT_C_API int anychat_friend_remove_from_blacklist(
    AnyChatFriendHandle   handle,
    const char*           user_id,
    void*                 userdata,
    AnyChatFriendCallback callback);

/* ---- Incoming event handlers ---- */

ANYCHAT_C_API void anychat_friend_set_request_callback(
    AnyChatFriendHandle          handle,
    void*                        userdata,
    AnyChatFriendRequestCallback callback);

ANYCHAT_C_API void anychat_friend_set_list_changed_callback(
    AnyChatFriendHandle              handle,
    void*                            userdata,
    AnyChatFriendListChangedCallback callback);

#ifdef __cplusplus
}
#endif
