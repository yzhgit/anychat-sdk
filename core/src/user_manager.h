#pragma once

#include "sdk_callbacks.h"
#include "sdk_types.h"

#include "network/http_client.h"

#include <memory>
#include <mutex>
#include <string>

namespace anychat {

class NotificationManager;
struct NotificationEvent;

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

class UserManagerImpl {
public:
    explicit UserManagerImpl(
        std::shared_ptr<network::HttpClient> http,
        NotificationManager* notif_mgr = nullptr,
        std::string device_id = {}
    );

    void getProfile(AnyChatValueCallback<UserProfile> callback);
    void updateProfile(const UserProfile& profile, AnyChatValueCallback<UserProfile> callback);
    void getSettings(AnyChatValueCallback<UserSettings> callback);
    void updateSettings(const UserSettings& settings, AnyChatValueCallback<UserSettings> callback);
    void updatePushToken(const std::string& push_token, int32_t platform, AnyChatCallback callback);
    void updatePushToken(
        const std::string& push_token,
        int32_t platform,
        const std::string& device_id,
        AnyChatCallback callback
    );
    void searchUsers(
        const std::string& keyword,
        int page,
        int page_size,
        AnyChatValueCallback<UserSearchResult> callback
    );
    void getUserInfo(const std::string& user_id, AnyChatValueCallback<UserInfo> callback);
    void bindPhone(
        const std::string& phone_number,
        const std::string& verify_code,
        AnyChatValueCallback<BindPhoneResult> callback
    );
    void changePhone(
        const std::string& old_phone_number,
        const std::string& new_phone_number,
        const std::string& new_verify_code,
        const std::string& old_verify_code,
        AnyChatValueCallback<ChangePhoneResult> callback
    );
    void bindEmail(
        const std::string& email,
        const std::string& verify_code,
        AnyChatValueCallback<BindEmailResult> callback
    );
    void changeEmail(
        const std::string& old_email,
        const std::string& new_email,
        const std::string& new_verify_code,
        const std::string& old_verify_code,
        AnyChatValueCallback<ChangeEmailResult> callback
    );
    void refreshQRCode(AnyChatValueCallback<UserQRCode> callback);
    void getUserByQRCode(const std::string& qrcode, AnyChatValueCallback<UserInfo> callback);
    void setListener(std::shared_ptr<UserListener> listener);

private:
    static std::string urlEncode(const std::string& input);
    void handleUserNotification(const NotificationEvent& event);

    std::shared_ptr<network::HttpClient> http_;
    NotificationManager* notif_mgr_ = nullptr;
    std::string device_id_;

    mutable std::mutex handler_mutex_;
    std::shared_ptr<UserListener> listener_;
};

} // namespace anychat
