#include "handles_c.h"
#include "anychat_c/user_c.h"
#include "utils_c.h"

#include <cstdlib>
#include <cstring>

namespace {

void userInfoToC(const anychat::UserInfo& src, AnyChatUserInfo_C* dst) {
    anychat_strlcpy(dst->user_id,    src.user_id.c_str(),    sizeof(dst->user_id));
    anychat_strlcpy(dst->username,   src.username.c_str(),   sizeof(dst->username));
    anychat_strlcpy(dst->avatar_url, src.avatar_url.c_str(), sizeof(dst->avatar_url));
}

void profileToC(const anychat::UserProfile& src, AnyChatUserProfile_C* dst) {
    anychat_strlcpy(dst->user_id,    src.user_id.c_str(),    sizeof(dst->user_id));
    anychat_strlcpy(dst->nickname,   src.nickname.c_str(),   sizeof(dst->nickname));
    anychat_strlcpy(dst->avatar_url, src.avatar_url.c_str(), sizeof(dst->avatar_url));
    anychat_strlcpy(dst->phone,      src.phone.c_str(),      sizeof(dst->phone));
    anychat_strlcpy(dst->email,      src.email.c_str(),      sizeof(dst->email));
    anychat_strlcpy(dst->signature,  src.signature.c_str(),  sizeof(dst->signature));
    anychat_strlcpy(dst->region,     src.region.c_str(),     sizeof(dst->region));
    dst->gender        = src.gender;
    dst->created_at_ms = src.created_at_ms;
}

void profileFromC(const AnyChatUserProfile_C* src, anychat::UserProfile& dst) {
    dst.user_id    = src->user_id;
    dst.nickname   = src->nickname;
    dst.avatar_url = src->avatar_url;
    dst.phone      = src->phone;
    dst.email      = src->email;
    dst.signature  = src->signature;
    dst.region     = src->region;
    dst.gender     = src->gender;
}

void settingsToC(const anychat::UserSettings& src, AnyChatUserSettings_C* dst) {
    dst->notification_enabled    = src.notification_enabled    ? 1 : 0;
    dst->sound_enabled           = src.sound_enabled           ? 1 : 0;
    dst->vibration_enabled       = src.vibration_enabled       ? 1 : 0;
    dst->message_preview_enabled = src.message_preview_enabled ? 1 : 0;
    dst->friend_verify_required  = src.friend_verify_required  ? 1 : 0;
    dst->search_by_phone         = src.search_by_phone         ? 1 : 0;
    dst->search_by_id            = src.search_by_id            ? 1 : 0;
    anychat_strlcpy(dst->language, src.language.c_str(), sizeof(dst->language));
}

void settingsFromC(const AnyChatUserSettings_C* src, anychat::UserSettings& dst) {
    dst.notification_enabled    = src->notification_enabled    != 0;
    dst.sound_enabled           = src->sound_enabled           != 0;
    dst.vibration_enabled       = src->vibration_enabled       != 0;
    dst.message_preview_enabled = src->message_preview_enabled != 0;
    dst.friend_verify_required  = src->friend_verify_required  != 0;
    dst.search_by_phone         = src->search_by_phone         != 0;
    dst.search_by_id            = src->search_by_id            != 0;
    dst.language                = src->language;
}

} // namespace

extern "C" {

int anychat_user_get_profile(
    AnyChatUserHandle          handle,
    void*                      userdata,
    AnyChatUserProfileCallback callback)
{
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->getProfile(
        [userdata, callback](bool ok, const anychat::UserProfile& p, const std::string& err) {
            if (!callback) return;
            if (ok) {
                AnyChatUserProfile_C c_p{};
                profileToC(p, &c_p);
                callback(userdata, 1, &c_p, "");
            } else {
                callback(userdata, 0, nullptr, err.c_str());
            }
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_user_update_profile(
    AnyChatUserHandle           handle,
    const AnyChatUserProfile_C* profile,
    void*                       userdata,
    AnyChatUserProfileCallback  callback)
{
    if (!handle || !handle->impl || !profile) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    anychat::UserProfile cpp_profile;
    profileFromC(profile, cpp_profile);
    handle->impl->updateProfile(cpp_profile,
        [userdata, callback](bool ok, const anychat::UserProfile& p, const std::string& err) {
            if (!callback) return;
            if (ok) {
                AnyChatUserProfile_C c_p{};
                profileToC(p, &c_p);
                callback(userdata, 1, &c_p, "");
            } else {
                callback(userdata, 0, nullptr, err.c_str());
            }
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_user_get_settings(
    AnyChatUserHandle           handle,
    void*                       userdata,
    AnyChatUserSettingsCallback callback)
{
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->getSettings(
        [userdata, callback](bool ok, const anychat::UserSettings& s, const std::string& err) {
            if (!callback) return;
            if (ok) {
                AnyChatUserSettings_C c_s{};
                settingsToC(s, &c_s);
                callback(userdata, 1, &c_s, "");
            } else {
                callback(userdata, 0, nullptr, err.c_str());
            }
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_user_update_settings(
    AnyChatUserHandle            handle,
    const AnyChatUserSettings_C* settings,
    void*                        userdata,
    AnyChatUserSettingsCallback  callback)
{
    if (!handle || !handle->impl || !settings) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    anychat::UserSettings cpp_settings;
    settingsFromC(settings, cpp_settings);
    handle->impl->updateSettings(cpp_settings,
        [userdata, callback](bool ok, const anychat::UserSettings& s, const std::string& err) {
            if (!callback) return;
            if (ok) {
                AnyChatUserSettings_C c_s{};
                settingsToC(s, &c_s);
                callback(userdata, 1, &c_s, "");
            } else {
                callback(userdata, 0, nullptr, err.c_str());
            }
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_user_update_push_token(
    AnyChatUserHandle         handle,
    const char*               push_token,
    const char*               platform,
    void*                     userdata,
    AnyChatUserResultCallback callback)
{
    if (!handle || !handle->impl || !push_token || !platform) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->updatePushToken(push_token, platform,
        [userdata, callback](bool ok, const std::string& err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_user_search(
    AnyChatUserHandle       handle,
    const char*             keyword,
    int                     page,
    int                     page_size,
    void*                   userdata,
    AnyChatUserListCallback callback)
{
    if (!handle || !handle->impl || !keyword) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->searchUsers(keyword, page, page_size,
        [userdata, callback](const std::vector<anychat::UserInfo>& users,
                             int64_t total, const std::string& err)
        {
            if (!callback) return;
            int count = static_cast<int>(users.size());
            AnyChatUserList_C c_list{};
            c_list.count = count;
            c_list.total = total;
            c_list.items = count > 0
                ? static_cast<AnyChatUserInfo_C*>(std::calloc(count, sizeof(AnyChatUserInfo_C)))
                : nullptr;
            for (int i = 0; i < count; ++i) userInfoToC(users[i], &c_list.items[i]);
            callback(userdata, &c_list, err.empty() ? nullptr : err.c_str());
            std::free(c_list.items);
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_user_get_info(
    AnyChatUserHandle       handle,
    const char*             user_id,
    void*                   userdata,
    AnyChatUserInfoCallback callback)
{
    if (!handle || !handle->impl || !user_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->getUserInfo(user_id,
        [userdata, callback](bool ok, const anychat::UserInfo& info, const std::string& err) {
            if (!callback) return;
            if (ok) {
                AnyChatUserInfo_C c_info{};
                userInfoToC(info, &c_info);
                callback(userdata, 1, &c_info, "");
            } else {
                callback(userdata, 0, nullptr, err.c_str());
            }
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

} // extern "C"
