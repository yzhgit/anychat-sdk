#pragma once

#include "anychat/user.h"
#include "network/http_client.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <string>

namespace anychat {

class UserManagerImpl : public UserManager {
public:
    explicit UserManagerImpl(std::shared_ptr<network::HttpClient> http);

    void getProfile(ProfileCallback callback) override;
    void updateProfile(const UserProfile& profile, ProfileCallback callback) override;
    void getSettings(SettingsCallback callback) override;
    void updateSettings(const UserSettings& settings, SettingsCallback callback) override;
    void updatePushToken(const std::string& push_token,
                          const std::string& platform,
                          ResultCallback callback) override;
    void searchUsers(const std::string& keyword,
                      int page, int page_size,
                      UserListCallback callback) override;
    void getUserInfo(const std::string& user_id, UserInfoCallback callback) override;

private:
    static UserProfile  parseProfile(const nlohmann::json& j);
    static UserSettings parseSettings(const nlohmann::json& j);
    static UserInfo     parseUserInfo(const nlohmann::json& j);

    std::shared_ptr<network::HttpClient> http_;
};

} // namespace anychat
