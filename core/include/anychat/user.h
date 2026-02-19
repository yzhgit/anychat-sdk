#pragma once

#include "types.h"

#include <functional>
#include <string>
#include <vector>

namespace anychat {

class UserManager {
public:
    using ProfileCallback  = std::function<void(bool ok, const UserProfile&,  const std::string& err)>;
    using SettingsCallback = std::function<void(bool ok, const UserSettings&, const std::string& err)>;
    using UserInfoCallback = std::function<void(bool ok, const UserInfo&,     const std::string& err)>;
    using UserListCallback = std::function<void(const std::vector<UserInfo>& users,
                                                int64_t total, const std::string& err)>;
    using ResultCallback   = std::function<void(bool ok, const std::string& err)>;

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
    virtual void updatePushToken(const std::string& push_token,
                                  const std::string& platform,
                                  ResultCallback callback) = 0;

    // GET  /users/search?keyword=&page=&pageSize=
    virtual void searchUsers(const std::string& keyword,
                              int page, int page_size,
                              UserListCallback callback) = 0;

    // GET  /users/{userId}
    virtual void getUserInfo(const std::string& user_id, UserInfoCallback callback) = 0;
};

} // namespace anychat
