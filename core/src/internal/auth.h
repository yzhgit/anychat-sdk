#pragma once

#include "callbacks.h"
#include "types.h"

#include <memory>
#include <string>
#include <vector>

namespace anychat {

class AuthListener {
public:
    virtual ~AuthListener() = default;

    virtual void onAuthExpired() {}
};

class AuthManager {
public:
    virtual ~AuthManager() = default;

    // Register a new account.
    // verify_code: SMS / email verification code.
    // nickname: optional display name (pass empty string to skip).
    // client_version: client version string (e.g. "1.0.0")
    virtual void registerUser(
        const std::string& phone_or_email,
        const std::string& password,
        const std::string& verify_code,
        int32_t device_type,
        const std::string& nickname,
        const std::string& client_version,
        AnyChatValueCallback<AuthToken> callback
    ) = 0;

    // Send verification code for register/reset_password/bind/change flows.
    // target_type: ANYCHAT_VERIFY_TARGET_*
    // purpose: ANYCHAT_VERIFY_PURPOSE_*
    virtual void sendVerificationCode(
        const std::string& target,
        int32_t target_type,
        int32_t purpose,
        AnyChatValueCallback<VerificationCodeResult> callback
    ) = 0;

    // Login with account (phone number or email) + password.
    // device_type: ANYCHAT_DEVICE_TYPE_*
    // client_version: client version string (e.g. "1.0.0")
    // The manager uses the device_id provided at construction time.
    virtual void login(
        const std::string& account,
        const std::string& password,
        int32_t device_type,
        const std::string& client_version,
        AnyChatValueCallback<AuthToken> callback
    ) = 0;

    // Logout the current device and invalidate its token.
    virtual void logout(AnyChatCallback callback) = 0;

    // Exchange a refresh_token for a new AccessToken.
    virtual void refreshToken(const std::string& refresh_token, AnyChatValueCallback<AuthToken> callback) = 0;

    // Change the current user's password (requires a valid access_token).
    virtual void
    changePassword(const std::string& old_password, const std::string& new_password, AnyChatCallback callback) = 0;

    // Reset password (forgot password flow).
    virtual void resetPassword(
        const std::string& account,
        const std::string& verify_code,
        const std::string& new_password,
        AnyChatCallback callback
    ) = 0;

    // Query logged-in devices for current user.
    virtual void getDeviceList(AnyChatValueCallback<std::vector<AuthDevice>> callback) = 0;

    // Force logout a specific device by device_id.
    virtual void logoutDevice(const std::string& device_id, AnyChatCallback callback) = 0;

    virtual bool isLoggedIn() const = 0;
    virtual AuthToken currentToken() const = 0;

    // Checks token expiry; if expired (or about to expire) refreshes automatically.
    virtual void ensureValidToken(AnyChatCallback cb) = 0;

    // Fired when the access token has expired and cannot be refreshed
    // (e.g. refresh token also invalid). The client must re-login.
    virtual void setListener(std::shared_ptr<AuthListener> listener) = 0;
};

} // namespace anychat
