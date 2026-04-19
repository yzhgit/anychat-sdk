#pragma once

#include "notification_manager.h"
#include "sdk_callbacks.h"
#include "sdk_types.h"

#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace anychat {

class AuthListener {
public:
    virtual ~AuthListener() = default;

    virtual void onAuthExpired() {}
};

class AuthManagerImpl {
public:
    // |http|      - shared HTTP client (base_url already set to api_base_url).
    // |device_id| - stable identifier for this installation/device.
    // |db|        - optional database for token persistence; may be nullptr.
    AuthManagerImpl(
        std::shared_ptr<network::HttpClient> http,
        std::string device_id,
        db::Database* db,
        NotificationManager* notif_mgr
    );

    void registerUser(
        const std::string& phone_or_email,
        const std::string& password,
        const std::string& verify_code,
        int32_t device_type,
        const std::string& nickname,
        const std::string& client_version,
        AnyChatValueCallback<AuthToken> callback
    );

    void sendVerificationCode(
        const std::string& target,
        int32_t target_type,
        int32_t purpose,
        AnyChatValueCallback<VerificationCodeResult> callback
    );

    void login(
        const std::string& account,
        const std::string& password,
        int32_t device_type,
        const std::string& client_version,
        AnyChatValueCallback<AuthToken> callback
    );

    void logout(AnyChatCallback callback);

    void refreshToken(const std::string& refresh_token, AnyChatValueCallback<AuthToken> callback);

    void changePassword(const std::string& old_password, const std::string& new_password, AnyChatCallback callback);

    void resetPassword(
        const std::string& account,
        const std::string& verify_code,
        const std::string& new_password,
        AnyChatCallback callback
    );

    void getDeviceList(AnyChatValueCallback<std::vector<AuthDevice>> callback);
    void logoutDevice(const std::string& device_id, AnyChatCallback callback);

    bool isLoggedIn() const;
    AuthToken currentToken() const;

    void ensureValidToken(AnyChatCallback cb);
    void setListener(std::shared_ptr<AuthListener> listener);

private:
    void handleAuthResponse(network::HttpResponse resp, const AnyChatValueCallback<AuthToken>& callback);
    void
    handleResultResponse(network::HttpResponse resp, const std::string& fallback_message, const AnyChatCallback& cb);
    void handleAuthNotification(const NotificationEvent& event);
    void storeToken(const AuthToken& token);
    void clearToken();

    std::shared_ptr<network::HttpClient> http_;
    std::string device_id_;
    db::Database* db_; // non-owning, may be nullptr
    mutable std::mutex token_mutex_;
    AuthToken token_;
    std::mutex listener_mutex_;
    std::shared_ptr<AuthListener> listener_;
};

} // namespace anychat
