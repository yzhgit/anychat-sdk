#pragma once

#include "callbacks.h"
#include "types.h"

#include <memory>
#include <string>

namespace anychat {

class UserListener {
public:
    virtual ~UserListener() = default;

    virtual void onProfileUpdated(const UserInfo& info) {
        (void) info;
    }

    virtual void onFriendProfileChanged(const UserInfo& info) {
        (void) info;
    }

    virtual void onUserStatusChanged(const UserStatusEvent& event) {
        (void) event;
    }
};

class UserManager {
public:
    virtual ~UserManager() = default;

    // GET  /users/me
    virtual void getProfile(AnyChatValueCallback<UserProfile> callback) = 0;

    // PUT  /users/me
    virtual void updateProfile(const UserProfile& profile, AnyChatValueCallback<UserProfile> callback) = 0;

    // GET  /users/me/settings
    virtual void getSettings(AnyChatValueCallback<UserSettings> callback) = 0;

    // PUT  /users/me/settings
    virtual void updateSettings(const UserSettings& settings, AnyChatValueCallback<UserSettings> callback) = 0;

    // POST /users/me/push-token
    virtual void
    updatePushToken(const std::string& push_token, const std::string& platform, AnyChatCallback callback) = 0;

    // POST /users/me/push-token
    virtual void updatePushToken(
        const std::string& push_token,
        const std::string& platform,
        const std::string& device_id,
        AnyChatCallback callback
    ) = 0;

    // GET  /users/search?keyword=&page=&pageSize=
    virtual void searchUsers(
        const std::string& keyword,
        int page,
        int page_size,
        AnyChatValueCallback<UserSearchResult> callback
    ) = 0;

    // GET  /users/{userId}
    virtual void getUserInfo(const std::string& user_id, AnyChatValueCallback<UserInfo> callback) = 0;

    // POST /users/me/phone/bind
    virtual void bindPhone(
        const std::string& phone_number,
        const std::string& verify_code,
        AnyChatValueCallback<BindPhoneResult> callback
    ) = 0;

    // POST /users/me/phone/change
    virtual void changePhone(
        const std::string& old_phone_number,
        const std::string& new_phone_number,
        const std::string& new_verify_code,
        const std::string& old_verify_code,
        AnyChatValueCallback<ChangePhoneResult> callback
    ) = 0;

    // POST /users/me/email/bind
    virtual void bindEmail(
        const std::string& email,
        const std::string& verify_code,
        AnyChatValueCallback<BindEmailResult> callback
    ) = 0;

    // POST /users/me/email/change
    virtual void changeEmail(
        const std::string& old_email,
        const std::string& new_email,
        const std::string& new_verify_code,
        const std::string& old_verify_code,
        AnyChatValueCallback<ChangeEmailResult> callback
    ) = 0;

    // POST /users/me/qrcode/refresh
    virtual void refreshQRCode(AnyChatValueCallback<UserQRCode> callback) = 0;

    // GET /users/qrcode?qrcode=
    virtual void getUserByQRCode(const std::string& qrcode, AnyChatValueCallback<UserInfo> callback) = 0;

    // WebSocket notification listener.
    virtual void setListener(std::shared_ptr<UserListener> listener) = 0;
};

} // namespace anychat
