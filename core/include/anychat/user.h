#pragma once

#include "types.h"

#include <functional>
#include <string>
#include <vector>

namespace anychat {

class UserManager {
public:
    using ProfileCallback = std::function<void(bool ok, const UserProfile&, const std::string& err)>;
    using SettingsCallback = std::function<void(bool ok, const UserSettings&, const std::string& err)>;
    using UserInfoCallback = std::function<void(bool ok, const UserInfo&, const std::string& err)>;
    using UserListCallback =
        std::function<void(const std::vector<UserInfo>& users, int64_t total, const std::string& err)>;
    using QRCodeCallback = std::function<void(bool ok, const UserQRCode&, const std::string& err)>;
    using BindPhoneCallback = std::function<void(bool ok, const BindPhoneResult&, const std::string& err)>;
    using ChangePhoneCallback = std::function<void(bool ok, const ChangePhoneResult&, const std::string& err)>;
    using BindEmailCallback = std::function<void(bool ok, const BindEmailResult&, const std::string& err)>;
    using ChangeEmailCallback = std::function<void(bool ok, const ChangeEmailResult&, const std::string& err)>;
    using ResultCallback = std::function<void(bool ok, const std::string& err)>;
    using OnProfileUpdated = std::function<void(const UserInfo&)>;
    using OnFriendProfileChanged = std::function<void(const UserInfo&)>;
    using OnUserStatusChanged = std::function<void(const UserStatusEvent&)>;

    virtual ~UserManager() = default;

    // GET  /users/me
    virtual void getProfile(ProfileCallback callback) = 0;

    // PUT  /users/me
    virtual void updateProfile(const UserProfile& profile, ProfileCallback callback) = 0;

    // GET  /users/me/settings
    virtual void getSettings(SettingsCallback callback) = 0;

    // PUT  /users/me/settings
    virtual void updateSettings(const UserSettings& settings, SettingsCallback callback) = 0;

    // POST /users/me/push-token
    virtual void
    updatePushToken(const std::string& push_token, const std::string& platform, ResultCallback callback) = 0;

    // POST /users/me/push-token
    virtual void updatePushToken(
        const std::string& push_token,
        const std::string& platform,
        const std::string& device_id,
        ResultCallback callback
    ) = 0;

    // GET  /users/search?keyword=&page=&pageSize=
    virtual void searchUsers(const std::string& keyword, int page, int page_size, UserListCallback callback) = 0;

    // GET  /users/{userId}
    virtual void getUserInfo(const std::string& user_id, UserInfoCallback callback) = 0;

    // POST /users/me/phone/bind
    virtual void bindPhone(
        const std::string& phone_number,
        const std::string& verify_code,
        BindPhoneCallback callback
    ) = 0;

    // POST /users/me/phone/change
    virtual void changePhone(
        const std::string& old_phone_number,
        const std::string& new_phone_number,
        const std::string& new_verify_code,
        const std::string& old_verify_code,
        ChangePhoneCallback callback
    ) = 0;

    // POST /users/me/email/bind
    virtual void bindEmail(
        const std::string& email,
        const std::string& verify_code,
        BindEmailCallback callback
    ) = 0;

    // POST /users/me/email/change
    virtual void changeEmail(
        const std::string& old_email,
        const std::string& new_email,
        const std::string& new_verify_code,
        const std::string& old_verify_code,
        ChangeEmailCallback callback
    ) = 0;

    // POST /users/me/qrcode/refresh
    virtual void refreshQRCode(QRCodeCallback callback) = 0;

    // GET /users/qrcode?qrcode=
    virtual void getUserByQRCode(const std::string& qrcode, UserInfoCallback callback) = 0;

    // WebSocket notification handlers.
    virtual void setOnProfileUpdated(OnProfileUpdated handler) = 0;
    virtual void setOnFriendProfileChanged(OnFriendProfileChanged handler) = 0;
    virtual void setOnUserStatusChanged(OnUserStatusChanged handler) = 0;
};

} // namespace anychat
