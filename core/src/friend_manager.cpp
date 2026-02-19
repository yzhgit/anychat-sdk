#include "friend_manager.h"

#include <nlohmann/json.hpp>
#include <string>

namespace anychat {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Friend parseFriend(const nlohmann::json& item) {
    Friend f;
    f.user_id       = item.value("userId", "");
    f.remark        = item.value("remark", "");
    // updatedAt may be Unix seconds; convert to ms
    if (item.contains("updatedAt") && item["updatedAt"].is_number()) {
        f.updated_at_ms = item["updatedAt"].get<int64_t>() * 1000LL;
    }
    f.is_deleted = item.value("isDeleted", false);

    if (item.contains("userInfo") && item["userInfo"].is_object()) {
        const auto& ui  = item["userInfo"];
        f.user_info.user_id    = ui.value("userId", "");
        f.user_info.username   = ui.value("nickname", "");
        f.user_info.avatar_url = ui.value("avatarUrl", "");
    }
    return f;
}

static FriendRequest parseFriendRequest(const nlohmann::json& item) {
    FriendRequest r;
    r.request_id   = item.value("requestId", int64_t{0});
    r.from_user_id = item.value("fromUserId", "");
    r.to_user_id   = item.value("toUserId", "");
    r.message      = item.value("message", "");
    r.status       = item.value("status", "pending");
    if (item.contains("createdAt") && item["createdAt"].is_number()) {
        r.created_at_ms = item["createdAt"].get<int64_t>() * 1000LL;
    }
    if (item.contains("fromUserInfo") && item["fromUserInfo"].is_object()) {
        const auto& ui      = item["fromUserInfo"];
        r.from_user_info.user_id    = ui.value("userId", "");
        r.from_user_info.username   = ui.value("nickname", "");
        r.from_user_info.avatar_url = ui.value("avatarUrl", "");
    }
    return r;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

FriendManagerImpl::FriendManagerImpl(db::Database* db,
                                     NotificationManager* notif_mgr,
                                     std::shared_ptr<network::HttpClient> http)
    : db_(db)
    , notif_mgr_(notif_mgr)
    , http_(std::move(http))
{
    if (notif_mgr_) {
        notif_mgr_->addNotificationHandler(
            [this](const NotificationEvent& ev) {
                handleFriendNotification(ev);
            });
    }
}

// ---------------------------------------------------------------------------
// getList
// ---------------------------------------------------------------------------

void FriendManagerImpl::getList(FriendListCallback cb) {
    http_->get("/friends",
        [cb = std::move(cb), this](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb({}, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) != 0) {
                    cb({}, j.value("message", "server error"));
                    return;
                }
                std::vector<Friend> friends;
                const auto& data = j["data"];
                // Support both data.list and data as array
                const nlohmann::json* arr = nullptr;
                if (data.is_array()) {
                    arr = &data;
                } else if (data.is_object() && data.contains("list") && data["list"].is_array()) {
                    arr = &data["list"];
                }
                if (arr) {
                    for (const auto& item : *arr) {
                        Friend f = parseFriend(item);
                        friends.push_back(f);
                        // Upsert into DB
                        if (db_) {
                            db_->exec(
                                "INSERT OR REPLACE INTO friends"
                                " (user_id, remark, updated_at_ms, is_deleted,"
                                "  friend_nickname, friend_avatar)"
                                " VALUES (?, ?, ?, ?, ?, ?)",
                                {f.user_id,
                                 f.remark,
                                 f.updated_at_ms,
                                 f.is_deleted ? int64_t{1} : int64_t{0},
                                 f.user_info.username,
                                 f.user_info.avatar_url},
                                nullptr);
                        }
                    }
                }
                cb(std::move(friends), "");
            } catch (const std::exception& e) {
                cb({}, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// sendRequest
// ---------------------------------------------------------------------------

void FriendManagerImpl::sendRequest(const std::string& to_user_id,
                                    const std::string& message,
                                    FriendCallback cb) {
    nlohmann::json body;
    body["toUserId"] = to_user_id;
    body["message"]  = message;

    http_->post("/friends/requests", body.dump(),
        [cb = std::move(cb)](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb(false, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) == 0) {
                    cb(true, "");
                } else {
                    cb(false, j.value("message", "server error"));
                }
            } catch (const std::exception& e) {
                cb(false, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// handleRequest
// ---------------------------------------------------------------------------

void FriendManagerImpl::handleRequest(int64_t request_id, bool accept, FriendCallback cb) {
    nlohmann::json body;
    body["accept"] = accept;

    std::string path = "/friends/requests/" + std::to_string(request_id);
    http_->put(path, body.dump(),
        [cb = std::move(cb)](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb(false, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) == 0) {
                    cb(true, "");
                } else {
                    cb(false, j.value("message", "server error"));
                }
            } catch (const std::exception& e) {
                cb(false, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// getPendingRequests
// ---------------------------------------------------------------------------

void FriendManagerImpl::getPendingRequests(FriendRequestListCallback cb) {
    http_->get("/friends/requests?type=received",
        [cb = std::move(cb)](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb({}, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) != 0) {
                    cb({}, j.value("message", "server error"));
                    return;
                }
                std::vector<FriendRequest> requests;
                const auto& data = j["data"];
                const nlohmann::json* arr = nullptr;
                if (data.is_array()) {
                    arr = &data;
                } else if (data.is_object() && data.contains("list") && data["list"].is_array()) {
                    arr = &data["list"];
                }
                if (arr) {
                    for (const auto& item : *arr) {
                        requests.push_back(parseFriendRequest(item));
                    }
                }
                cb(std::move(requests), "");
            } catch (const std::exception& e) {
                cb({}, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// deleteFriend
// ---------------------------------------------------------------------------

void FriendManagerImpl::deleteFriend(const std::string& friend_id, FriendCallback cb) {
    std::string path = "/friends/" + friend_id;
    http_->del(path,
        [cb = std::move(cb)](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb(false, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) == 0) {
                    cb(true, "");
                } else {
                    cb(false, j.value("message", "server error"));
                }
            } catch (const std::exception& e) {
                cb(false, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// updateRemark
// ---------------------------------------------------------------------------

void FriendManagerImpl::updateRemark(const std::string& friend_id,
                                     const std::string& remark,
                                     FriendCallback cb) {
    nlohmann::json body;
    body["remark"] = remark;

    std::string path = "/friends/" + friend_id + "/remark";
    http_->put(path, body.dump(),
        [cb = std::move(cb)](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb(false, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) == 0) {
                    cb(true, "");
                } else {
                    cb(false, j.value("message", "server error"));
                }
            } catch (const std::exception& e) {
                cb(false, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// addToBlacklist
// ---------------------------------------------------------------------------

void FriendManagerImpl::addToBlacklist(const std::string& user_id, FriendCallback cb) {
    nlohmann::json body;
    body["userId"] = user_id;

    http_->post("/friends/blacklist", body.dump(),
        [cb = std::move(cb)](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb(false, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) == 0) {
                    cb(true, "");
                } else {
                    cb(false, j.value("message", "server error"));
                }
            } catch (const std::exception& e) {
                cb(false, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// removeFromBlacklist
// ---------------------------------------------------------------------------

void FriendManagerImpl::removeFromBlacklist(const std::string& user_id, FriendCallback cb) {
    std::string path = "/friends/blacklist/" + user_id;
    http_->del(path,
        [cb = std::move(cb)](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                cb(false, resp.error);
                return;
            }
            try {
                auto j = nlohmann::json::parse(resp.body);
                if (j.value("code", -1) == 0) {
                    cb(true, "");
                } else {
                    cb(false, j.value("message", "server error"));
                }
            } catch (const std::exception& e) {
                cb(false, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// setOnFriendRequest / setOnFriendListChanged
// ---------------------------------------------------------------------------

void FriendManagerImpl::setOnFriendRequest(OnFriendRequest handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    on_friend_request_ = std::move(handler);
}

void FriendManagerImpl::setOnFriendListChanged(OnFriendListChanged handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    on_friend_list_changed_ = std::move(handler);
}

// ---------------------------------------------------------------------------
// handleFriendNotification
// ---------------------------------------------------------------------------

void FriendManagerImpl::handleFriendNotification(const NotificationEvent& event) {
    const std::string& type = event.notification_type;

    // Only handle friend-related notification types
    if (type != "friend.request" &&
        type != "friend.request_handled" &&
        type != "friend.deleted" &&
        type != "friend.remark_updated" &&
        type != "friend.blacklist_changed") {
        return;
    }

    if (type == "friend.request") {
        // Build a FriendRequest from the notification data and fire on_friend_request_
        OnFriendRequest handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = on_friend_request_;
        }
        if (handler) {
            FriendRequest req;
            try {
                const auto& d = event.data;
                req.request_id   = d.value("requestId", int64_t{0});
                req.from_user_id = d.value("fromUserId", "");
                req.message      = d.value("message", "");
                req.status       = "pending";
                req.created_at_ms = event.timestamp * 1000LL;

                if (d.contains("fromUserInfo") && d["fromUserInfo"].is_object()) {
                    const auto& ui      = d["fromUserInfo"];
                    req.from_user_info.user_id    = ui.value("userId", "");
                    req.from_user_info.username   = ui.value("nickname", "");
                    req.from_user_info.avatar_url = ui.value("avatarUrl", "");
                }
            } catch (...) {}
            handler(req);
        }
    } else {
        // friend.request_handled, friend.deleted, friend.remark_updated,
        // friend.blacklist_changed — notify that the list changed
        OnFriendListChanged handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = on_friend_list_changed_;
        }
        if (handler) {
            handler();
        }
    }
}

} // namespace anychat
