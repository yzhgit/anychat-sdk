#pragma once

#include "types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Callback types ---- */

typedef void (*AnyChatConvListCallback)(
    void*                             userdata,
    const AnyChatConversationList_C*  list,
    const char*                       error);

typedef void (*AnyChatConvCallback)(
    void*       userdata,
    int         success,
    const char* error);

/* Fired when any conversation is created or updated. */
typedef void (*AnyChatConvUpdatedCallback)(
    void*                        userdata,
    const AnyChatConversation_C* conversation);

/* ---- Conversation operations ---- */

/* Return cached + DB list (pinned first, then by last_msg_time desc). */
ANYCHAT_C_API int anychat_conv_get_list(
    AnyChatConvHandle       handle,
    void*                   userdata,
    AnyChatConvListCallback callback);

/* Mark a conversation as read. */
ANYCHAT_C_API int anychat_conv_mark_read(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    void*               userdata,
    AnyChatConvCallback callback);

/* Pin or unpin a conversation (pinned = 1, unpinned = 0). */
ANYCHAT_C_API int anychat_conv_set_pinned(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    int                 pinned,
    void*               userdata,
    AnyChatConvCallback callback);

/* Mute or unmute a conversation (muted = 1, unmuted = 0). */
ANYCHAT_C_API int anychat_conv_set_muted(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    int                 muted,
    void*               userdata,
    AnyChatConvCallback callback);

/* Delete a conversation (local + server). */
ANYCHAT_C_API int anychat_conv_delete(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    void*               userdata,
    AnyChatConvCallback callback);

/* Register a callback for conversation updates.
 * Pass NULL to clear. */
ANYCHAT_C_API void anychat_conv_set_updated_callback(
    AnyChatConvHandle          handle,
    void*                      userdata,
    AnyChatConvUpdatedCallback callback);

#ifdef __cplusplus
}
#endif
