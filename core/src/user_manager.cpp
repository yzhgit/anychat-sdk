#include "user_manager.h"

#include <nlohmann/json.hpp>
#include <string>

namespace anychat {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UserManagerImpl::UserManagerImpl(std::shared_ptr<network::HttpClient> http)
    : http_(std::move(http))
{}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/*static*/ UserProfile UserManagerImpl::parseProfile(const nlohmann::json& j)
{
    UserProfile p;
    p.user_id     = j.value("userId",    "");
    p.nickname    = j.value("nickname",  "");
    p.avatar_url  = j.value("avatar",    "");
    p.phone       = j.value("phone",     "");
    p.email       = j.value("email",     "");
    p.signature   = j.value("signature", "");
    p.region      = j.value("region",    "");
    p.gender      = j.value("gender",    0);
    // createdAt may be an ISO string; store raw for now (binding layer converts)
    return p;
}

/*static*/ UserSettings UserManagerImpl::parseSettings(const nlohmann::json& j)
{
    UserSettings s;
    s.notification_enabled    = j.value("notificationEnabled",   true);
    s.sound_enabled           = j.value("soundEnabled",          true);
    s.vibration_enabled       = j.value("vibrationEnabled",      true);
    s.message_preview_enabled = j.value("messagePreviewEnabled", true);
    s.friend_verify_required  = j.value("friendVerifyRequired",  false);
    s.search_by_phone         = j.value("searchByPhone",         true);
    s.search_by_id            = j.value("searchById",            true);
    s.language                = j.value("language",              "");
    return s;
}

/*static*/ UserInfo UserManagerImpl::parseUserInfo(const nlohmann::json& j)
{
    UserInfo u;
    u.user_id    = j.value("userId",   "");
    u.username   = j.value("nickname", "");
    u.avatar_url = j.value("avatar",   "");
    return u;
}

// ---------------------------------------------------------------------------
// getProfile
// ---------------------------------------------------------------------------

void UserManagerImpl::getProfile(ProfileCallback callback)
{
    http_->get("/users/me", [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) { cb(false, {}, resp.error); return; }
        try {
            auto root = nlohmann::json::parse(resp.body);
            if (root.value("code", -1) != 0) {
                cb(false, {}, root.value("message", "server error"));
                return;
            }
            cb(true, parseProfile(root["data"]), "");
        } catch (const std::exception& e) {
            cb(false, {}, std::string("parse error: ") + e.what());
        }
    });
}

// ---------------------------------------------------------------------------
// updateProfile
// ---------------------------------------------------------------------------

void UserManagerImpl::updateProfile(const UserProfile& profile, ProfileCallback callback)
{
    nlohmann::json body;
    if (!profile.nickname.empty())  body["nickname"]  = profile.nickname;
    if (!profile.avatar_url.empty()) body["avatar"]   = profile.avatar_url;
    if (!profile.signature.empty()) body["signature"] = profile.signature;
    if (!profile.region.empty())    body["region"]    = profile.region;
    if (profile.gender != 0)        body["gender"]    = profile.gender;

    http_->put("/users/me", body.dump(), [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) { cb(false, {}, resp.error); return; }
        try {
            auto root = nlohmann::json::parse(resp.body);
            if (root.value("code", -1) != 0) {
                cb(false, {}, root.value("message", "server error"));
                return;
            }
            cb(true, parseProfile(root["data"]), "");
        } catch (const std::exception& e) {
            cb(false, {}, std::string("parse error: ") + e.what());
        }
    });
}

// ---------------------------------------------------------------------------
// getSettings
// ---------------------------------------------------------------------------

void UserManagerImpl::getSettings(SettingsCallback callback)
{
    http_->get("/users/me/settings", [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) { cb(false, {}, resp.error); return; }
        try {
            auto root = nlohmann::json::parse(resp.body);
            if (root.value("code", -1) != 0) {
                cb(false, {}, root.value("message", "server error"));
                return;
            }
            cb(true, parseSettings(root["data"]), "");
        } catch (const std::exception& e) {
            cb(false, {}, std::string("parse error: ") + e.what());
        }
    });
}

// ---------------------------------------------------------------------------
// updateSettings
// ---------------------------------------------------------------------------

void UserManagerImpl::updateSettings(const UserSettings& settings, SettingsCallback callback)
{
    nlohmann::json body;
    body["notificationEnabled"]   = settings.notification_enabled;
    body["soundEnabled"]          = settings.sound_enabled;
    body["vibrationEnabled"]      = settings.vibration_enabled;
    body["messagePreviewEnabled"] = settings.message_preview_enabled;
    body["friendVerifyRequired"]  = settings.friend_verify_required;
    body["searchByPhone"]         = settings.search_by_phone;
    body["searchById"]            = settings.search_by_id;
    if (!settings.language.empty()) body["language"] = settings.language;

    http_->put("/users/me/settings", body.dump(),
        [cb = std::move(callback)](network::HttpResponse resp) {
            if (!resp.error.empty()) { cb(false, {}, resp.error); return; }
            try {
                auto root = nlohmann::json::parse(resp.body);
                if (root.value("code", -1) != 0) {
                    cb(false, {}, root.value("message", "server error"));
                    return;
                }
                cb(true, parseSettings(root["data"]), "");
            } catch (const std::exception& e) {
                cb(false, {}, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// updatePushToken
// ---------------------------------------------------------------------------

void UserManagerImpl::updatePushToken(const std::string& push_token,
                                       const std::string& platform,
                                       ResultCallback callback)
{
    nlohmann::json body;
    body["pushToken"] = push_token;
    body["platform"]  = platform;
    body["deviceId"]  = "";  // client.cpp can set device_id via http header

    http_->post("/users/me/push-token", body.dump(),
        [cb = std::move(callback)](network::HttpResponse resp) {
            if (!resp.error.empty()) { cb(false, resp.error); return; }
            try {
                auto root = nlohmann::json::parse(resp.body);
                bool ok = (root.value("code", -1) == 0);
                cb(ok, ok ? "" : root.value("message", "server error"));
            } catch (const std::exception& e) {
                cb(false, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// searchUsers
// ---------------------------------------------------------------------------

void UserManagerImpl::searchUsers(const std::string& keyword,
                                   int page, int page_size,
                                   UserListCallback callback)
{
    std::string path = "/users/search?keyword=" + keyword
                     + "&page=" + std::to_string(page)
                     + "&pageSize=" + std::to_string(page_size);

    http_->get(path, [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) { cb({}, 0, resp.error); return; }
        try {
            auto root = nlohmann::json::parse(resp.body);
            if (root.value("code", -1) != 0) {
                cb({}, 0, root.value("message", "server error"));
                return;
            }
            const auto& data  = root["data"];
            int64_t     total = data.value("total", int64_t{0});
            std::vector<UserInfo> users;
            for (const auto& item : data["users"]) {
                users.push_back(parseUserInfo(item));
            }
            cb(users, total, "");
        } catch (const std::exception& e) {
            cb({}, 0, std::string("parse error: ") + e.what());
        }
    });
}

// ---------------------------------------------------------------------------
// getUserInfo
// ---------------------------------------------------------------------------

void UserManagerImpl::getUserInfo(const std::string& user_id, UserInfoCallback callback)
{
    http_->get("/users/" + user_id,
        [cb = std::move(callback)](network::HttpResponse resp) {
            if (!resp.error.empty()) { cb(false, {}, resp.error); return; }
            try {
                auto root = nlohmann::json::parse(resp.body);
                if (root.value("code", -1) != 0) {
                    cb(false, {}, root.value("message", "server error"));
                    return;
                }
                cb(true, parseUserInfo(root["data"]), "");
            } catch (const std::exception& e) {
                cb(false, {}, std::string("parse error: ") + e.what());
            }
        });
}

} // namespace anychat
