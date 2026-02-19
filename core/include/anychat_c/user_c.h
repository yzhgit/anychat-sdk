#pragma once

#include "types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Callback types ---- */

typedef void (*AnyChatUserProfileCallback)(
    void*                       userdata,
    int                         success,
    const AnyChatUserProfile_C* profile,
    const char*                 error);

typedef void (*AnyChatUserSettingsCallback)(
    void*                        userdata,
    int                          success,
    const AnyChatUserSettings_C* settings,
    const char*                  error);

typedef void (*AnyChatUserInfoCallback)(
    void*                    userdata,
    int                      success,
    const AnyChatUserInfo_C* info,
    const char*              error);

typedef void (*AnyChatUserListCallback)(
    void*                    userdata,
    const AnyChatUserList_C* list,
    const char*              error);

typedef void (*AnyChatUserResultCallback)(
    void*       userdata,
    int         success,
    const char* error);

/* ---- User operations ---- */

ANYCHAT_C_API int anychat_user_get_profile(
    AnyChatUserHandle          handle,
    void*                      userdata,
    AnyChatUserProfileCallback callback);

ANYCHAT_C_API int anychat_user_update_profile(
    AnyChatUserHandle           handle,
    const AnyChatUserProfile_C* profile,
    void*                       userdata,
    AnyChatUserProfileCallback  callback);

ANYCHAT_C_API int anychat_user_get_settings(
    AnyChatUserHandle           handle,
    void*                       userdata,
    AnyChatUserSettingsCallback callback);

ANYCHAT_C_API int anychat_user_update_settings(
    AnyChatUserHandle            handle,
    const AnyChatUserSettings_C* settings,
    void*                        userdata,
    AnyChatUserSettingsCallback  callback);

/* Update the push notification token for this device.
 * platform: "ios" | "android" | "web" */
ANYCHAT_C_API int anychat_user_update_push_token(
    AnyChatUserHandle       handle,
    const char*             push_token,
    const char*             platform,
    void*                   userdata,
    AnyChatUserResultCallback callback);

/* Search for users by keyword (username / phone / e-mail). */
ANYCHAT_C_API int anychat_user_search(
    AnyChatUserHandle       handle,
    const char*             keyword,
    int                     page,
    int                     page_size,
    void*                   userdata,
    AnyChatUserListCallback callback);

/* Fetch public info for a specific user. */
ANYCHAT_C_API int anychat_user_get_info(
    AnyChatUserHandle      handle,
    const char*            user_id,
    void*                  userdata,
    AnyChatUserInfoCallback callback);

#ifdef __cplusplus
}
#endif
