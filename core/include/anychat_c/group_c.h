#pragma once

#include "types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Callback types ---- */

typedef void (*AnyChatGroupListCallback)(
    void*                    userdata,
    const AnyChatGroupList_C* list,
    const char*              error);

typedef void (*AnyChatGroupCallback)(
    void*       userdata,
    int         success,
    const char* error);

typedef void (*AnyChatGroupMemberCallback)(
    void*                          userdata,
    const AnyChatGroupMemberList_C* list,
    const char*                    error);

/* Fired when the current user is invited to a group. */
typedef void (*AnyChatGroupInvitedCallback)(
    void*                  userdata,
    const AnyChatGroup_C*  group,
    const char*            inviter_id);

/* Fired when group metadata changes. */
typedef void (*AnyChatGroupUpdatedCallback)(
    void*                 userdata,
    const AnyChatGroup_C* group);

/* ---- Group operations ---- */

ANYCHAT_C_API int anychat_group_get_list(
    AnyChatGroupHandle       handle,
    void*                    userdata,
    AnyChatGroupListCallback callback);

/* member_ids: NULL-terminated array of user ID strings.
 * member_count: length of the array (excluding the terminating NULL). */
ANYCHAT_C_API int anychat_group_create(
    AnyChatGroupHandle   handle,
    const char*          name,
    const char* const*   member_ids,
    int                  member_count,
    void*                userdata,
    AnyChatGroupCallback callback);

ANYCHAT_C_API int anychat_group_join(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    const char*          message,
    void*                userdata,
    AnyChatGroupCallback callback);

ANYCHAT_C_API int anychat_group_invite(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    const char* const*   user_ids,
    int                  user_count,
    void*                userdata,
    AnyChatGroupCallback callback);

ANYCHAT_C_API int anychat_group_quit(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    void*                userdata,
    AnyChatGroupCallback callback);

ANYCHAT_C_API int anychat_group_update(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    const char*          name,
    const char*          avatar_url,
    void*                userdata,
    AnyChatGroupCallback callback);

ANYCHAT_C_API int anychat_group_get_members(
    AnyChatGroupHandle         handle,
    const char*                group_id,
    int                        page,
    int                        page_size,
    void*                      userdata,
    AnyChatGroupMemberCallback callback);

/* ---- Incoming event handlers ---- */

ANYCHAT_C_API void anychat_group_set_invited_callback(
    AnyChatGroupHandle          handle,
    void*                       userdata,
    AnyChatGroupInvitedCallback callback);

ANYCHAT_C_API void anychat_group_set_updated_callback(
    AnyChatGroupHandle          handle,
    void*                       userdata,
    AnyChatGroupUpdatedCallback callback);

#ifdef __cplusplus
}
#endif
