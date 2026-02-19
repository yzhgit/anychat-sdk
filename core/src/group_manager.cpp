#include "group_manager.h"

#include <nlohmann/json.hpp>
#include <string>

namespace anychat {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static GroupRole parseRole(const std::string& role_str) {
    if (role_str == "owner") return GroupRole::Owner;
    if (role_str == "admin")  return GroupRole::Admin;
    return GroupRole::Member;
}

static Group parseGroup(const nlohmann::json& item) {
    Group g;
    g.group_id    = item.value("groupId", "");
    g.name        = item.value("name", "");
    g.avatar_url  = item.value("avatarUrl", item.value("avatar", ""));
    g.owner_id    = item.value("ownerId", "");
    g.member_count = item.value("memberCount", 0);
    g.my_role     = parseRole(item.value("myRole", "member"));
    g.join_verify = item.value("joinVerify", false);
    if (item.contains("updatedAt") && item["updatedAt"].is_number()) {
        g.updated_at_ms = item["updatedAt"].get<int64_t>() * 1000LL;
    }
    return g;
}

static GroupMember parseGroupMember(const nlohmann::json& item) {
    GroupMember m;
    m.user_id        = item.value("userId", "");
    m.group_nickname = item.value("groupNickname", "");
    m.role           = parseRole(item.value("role", "member"));
    m.is_muted       = item.value("isMuted", false);

    if (item.contains("joinedAt") && item["joinedAt"].is_number()) {
        m.joined_at_ms = item["joinedAt"].get<int64_t>() * 1000LL;
    }

    if (item.contains("userInfo") && item["userInfo"].is_object()) {
        const auto& ui    = item["userInfo"];
        m.user_info.user_id    = ui.value("userId", "");
        m.user_info.username   = ui.value("nickname", "");
        m.user_info.avatar_url = ui.value("avatarUrl", ui.value("avatar", ""));
    }
    return m;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

GroupManagerImpl::GroupManagerImpl(db::Database* db,
                                   NotificationManager* notif_mgr,
                                   std::shared_ptr<network::HttpClient> http)
    : db_(db)
    , notif_mgr_(notif_mgr)
    , http_(std::move(http))
{
    if (notif_mgr_) {
        notif_mgr_->addNotificationHandler(
            [this](const NotificationEvent& ev) {
                handleGroupNotification(ev);
            });
    }
}

// ---------------------------------------------------------------------------
// getList
// ---------------------------------------------------------------------------

void GroupManagerImpl::getList(GroupListCallback cb) {
    http_->get("/groups",
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
                std::vector<Group> groups;
                const auto& data = j["data"];
                const nlohmann::json* arr = nullptr;
                if (data.is_array()) {
                    arr = &data;
                } else if (data.is_object() && data.contains("groups") && data["groups"].is_array()) {
                    arr = &data["groups"];
                }
                if (arr) {
                    for (const auto& item : *arr) {
                        Group g = parseGroup(item);
                        groups.push_back(g);
                        // Upsert into DB
                        if (db_) {
                            db_->exec(
                                "INSERT OR REPLACE INTO groups"
                                " (group_id, name, avatar_url, owner_id,"
                                "  member_count, updated_at_ms)"
                                " VALUES (?, ?, ?, ?, ?, ?)",
                                {g.group_id,
                                 g.name,
                                 g.avatar_url,
                                 g.owner_id,
                                 static_cast<int64_t>(g.member_count),
                                 g.updated_at_ms},
                                nullptr);
                        }
                    }
                }
                cb(std::move(groups), "");
            } catch (const std::exception& e) {
                cb({}, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// create
// ---------------------------------------------------------------------------

void GroupManagerImpl::create(const std::string& name,
                              const std::vector<std::string>& member_ids,
                              GroupCallback cb) {
    nlohmann::json body;
    body["name"]      = name;
    body["memberIds"] = member_ids;

    http_->post("/groups", body.dump(),
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
// join
// ---------------------------------------------------------------------------

void GroupManagerImpl::join(const std::string& group_id,
                            const std::string& message,
                            GroupCallback cb) {
    nlohmann::json body;
    body["message"] = message;

    std::string path = "/groups/" + group_id + "/join";
    http_->post(path, body.dump(),
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
// invite
// ---------------------------------------------------------------------------

void GroupManagerImpl::invite(const std::string& group_id,
                              const std::vector<std::string>& user_ids,
                              GroupCallback cb) {
    nlohmann::json body;
    body["userIds"] = user_ids;

    std::string path = "/groups/" + group_id + "/members";
    http_->post(path, body.dump(),
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
// quit
// ---------------------------------------------------------------------------

void GroupManagerImpl::quit(const std::string& group_id, GroupCallback cb) {
    std::string path = "/groups/" + group_id + "/quit";
    http_->post(path, "{}",
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
// update
// ---------------------------------------------------------------------------

void GroupManagerImpl::update(const std::string& group_id,
                              const std::string& name,
                              const std::string& avatar_url,
                              GroupCallback cb) {
    nlohmann::json body;
    if (!name.empty())       body["name"]   = name;
    if (!avatar_url.empty()) body["avatar"] = avatar_url;

    std::string path = "/groups/" + group_id;
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
// getMembers
// ---------------------------------------------------------------------------

void GroupManagerImpl::getMembers(const std::string& group_id,
                                  int page,
                                  int page_size,
                                  GroupMemberCallback cb) {
    std::string path = "/groups/" + group_id + "/members"
                       + "?page=" + std::to_string(page)
                       + "&pageSize=" + std::to_string(page_size);

    http_->get(path,
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
                std::vector<GroupMember> members;
                const auto& data = j["data"];
                const nlohmann::json* arr = nullptr;
                if (data.is_array()) {
                    arr = &data;
                } else if (data.is_object() && data.contains("members") && data["members"].is_array()) {
                    arr = &data["members"];
                }
                if (arr) {
                    for (const auto& item : *arr) {
                        members.push_back(parseGroupMember(item));
                    }
                }
                cb(std::move(members), "");
            } catch (const std::exception& e) {
                cb({}, std::string("parse error: ") + e.what());
            }
        });
}

// ---------------------------------------------------------------------------
// setOnGroupInvited / setOnGroupUpdated
// ---------------------------------------------------------------------------

void GroupManagerImpl::setOnGroupInvited(OnGroupInvited handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    on_group_invited_ = std::move(handler);
}

void GroupManagerImpl::setOnGroupUpdated(OnGroupUpdated handler) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    on_group_updated_ = std::move(handler);
}

// ---------------------------------------------------------------------------
// handleGroupNotification
// ---------------------------------------------------------------------------

void GroupManagerImpl::handleGroupNotification(const NotificationEvent& event) {
    const std::string& type = event.notification_type;

    // Only handle group-related notification types
    if (type != "group.invited" &&
        type != "group.member_joined" &&
        type != "group.member_left" &&
        type != "group.info_updated" &&
        type != "group.role_changed" &&
        type != "group.disbanded") {
        return;
    }

    if (type == "group.invited") {
        OnGroupInvited handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = on_group_invited_;
        }
        if (handler) {
            Group g;
            std::string inviter_id;
            try {
                const auto& d = event.data;
                g.group_id    = d.value("groupId", "");
                g.name        = d.value("groupName", "");
                inviter_id    = d.value("inviterId", "");
            } catch (...) {}
            handler(g, inviter_id);
        }
    } else if (type == "group.info_updated") {
        OnGroupUpdated handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = on_group_updated_;
        }
        if (handler) {
            Group g;
            try {
                const auto& d = event.data;
                g.group_id   = d.value("groupId", "");
                g.name       = d.value("name", "");
                g.avatar_url = d.value("avatarUrl", d.value("avatar", ""));
                g.owner_id   = d.value("ownerId", "");
            } catch (...) {}
            handler(g);
        }
    } else {
        // group.member_joined, group.member_left, group.role_changed,
        // group.disbanded — notify via on_group_updated_ with partial data
        OnGroupUpdated handler;
        {
            std::lock_guard<std::mutex> lock(handler_mutex_);
            handler = on_group_updated_;
        }
        if (handler) {
            Group g;
            try {
                const auto& d = event.data;
                g.group_id = d.value("groupId", "");
            } catch (...) {}
            handler(g);
        }
    }
}

} // namespace anychat
