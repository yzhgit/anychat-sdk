#include "friend_manager.h"

#include "json_common.h"

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace anychat::friend_manager_detail {

using json_common::ApiEnvelope;
using json_common::parseApiEnvelopeResponse;
using json_common::parseInt32Value;
using json_common::parseJsonObject;
using json_common::parseTimestampMs;
using json_common::pickList;
using json_common::toLowerCopy;
using json_common::writeJson;

struct SendFriendRequestBody {
    std::string user_id{};
    std::string message{};
    int32_t source = 0;
};

using EnumValue = std::variant<int64_t, double, bool, std::string>;
using OptionalEnumValue = std::optional<EnumValue>;

struct HandleFriendRequestBody {
    int32_t action = 0;
};

struct UpdateRemarkBody {
    std::string remark{};
};

struct AddBlacklistBody {
    std::string user_id{};
};

struct NotificationUserInfoPayload {
    std::string user_id{};
    std::string nickname{};
    std::string avatar{};
    std::string signature{};
    int32_t gender = 0;
    std::string region{};
    bool is_friend = false;
    bool is_blocked = false;
};

struct NotificationFriendEventPayload {
    std::string user_id{};
    std::string friend_user_id{};
    std::string target_user_id{};
    std::string added_by_user_id{};
    std::string remark{};
    bool is_deleted = false;
    int64_t request_id = 0;
    std::string from_user_id{};
    std::string to_user_id{};
    std::string message{};
    OptionalEnumValue source{};
    OptionalEnumValue status{};
    std::string action{};
    json_common::OptionalTimestampValue updated_at{};
    json_common::OptionalTimestampValue created_at{};
    json_common::OptionalTimestampValue changed_at{};
    json_common::OptionalTimestampValue deleted_at{};
    json_common::OptionalTimestampValue handled_at{};
    std::optional<NotificationUserInfoPayload> user_info{};
    std::optional<NotificationUserInfoPayload> from_user_info{};
    std::optional<NotificationUserInfoPayload> blocked_user_info{};
};

struct FriendPayload {
    std::string user_id{};
    std::string remark{};
    json_common::OptionalTimestampValue updated_at{};
    bool is_deleted = false;
    std::optional<NotificationUserInfoPayload> user_info{};
};

struct FriendListDataPayload {
    std::optional<std::vector<FriendPayload>> friends{};
};

struct FriendRequestPayload {
    int64_t id = 0;
    std::string from_user_id{};
    std::string to_user_id{};
    std::string message{};
    OptionalEnumValue source{};
    OptionalEnumValue status{};
    json_common::OptionalTimestampValue created_at{};
    std::optional<NotificationUserInfoPayload> from_user_info{};
};

struct FriendRequestListDataPayload {
    std::optional<std::vector<FriendRequestPayload>> requests{};
};

struct BlacklistItemPayload {
    int64_t id = 0;
    std::string user_id{};
    std::string blocked_user_id{};
    json_common::OptionalTimestampValue created_at{};
    std::optional<NotificationUserInfoPayload> blocked_user_info{};
};

struct BlacklistListDataPayload {
    std::optional<std::vector<BlacklistItemPayload>> items{};
};

UserInfo toUserInfo(const NotificationUserInfoPayload& payload) {
    UserInfo info;
    info.user_id = payload.user_id;
    info.username = payload.nickname;
    info.avatar_url = payload.avatar;
    info.signature = payload.signature;
    info.gender = payload.gender;
    info.region = payload.region;
    info.is_friend = payload.is_friend;
    info.is_blocked = payload.is_blocked;
    return info;
}

constexpr int32_t kFriendSourceSearch = 1;
constexpr int32_t kFriendRequestStatusPending = 1;
constexpr int32_t kFriendRequestStatusAccepted = 2;
constexpr int32_t kFriendRequestStatusRejected = 3;
constexpr int32_t kFriendRequestStatusExpired = 4;
constexpr int32_t kFriendRequestActionAccept = 1;
constexpr int32_t kFriendRequestActionReject = 2;
constexpr int32_t kFriendRequestQueryTypeReceived = 1;
constexpr int32_t kFriendRequestQueryTypeSent = 2;

int32_t normalizeSource(const OptionalEnumValue& value) {
    return parseInt32Value(value, kFriendSourceSearch);
}

int32_t normalizeStatus(const OptionalEnumValue& value) {
    return parseInt32Value(value, kFriendRequestStatusPending);
}

Friend toFriend(const FriendPayload& payload) {
    Friend f;
    f.user_id = payload.user_id;
    f.remark = payload.remark;
    f.updated_at_ms = parseTimestampMs(payload.updated_at);
    f.is_deleted = payload.is_deleted;

    if (payload.user_info.has_value()) {
        f.user_info = toUserInfo(*payload.user_info);
    }
    if (f.user_info.user_id.empty()) {
        f.user_info.user_id = f.user_id;
    }
    return f;
}

FriendRequest toFriendRequest(const FriendRequestPayload& payload) {
    FriendRequest request;
    request.request_id = payload.id;
    request.from_user_id = payload.from_user_id;
    request.to_user_id = payload.to_user_id;
    request.message = payload.message;
    request.source = normalizeSource(payload.source);
    request.status = normalizeStatus(payload.status);
    request.created_at_ms = parseTimestampMs(payload.created_at);

    if (payload.from_user_info.has_value()) {
        request.from_user_info = toUserInfo(*payload.from_user_info);
    }
    if (request.from_user_info.user_id.empty()) {
        request.from_user_info.user_id = request.from_user_id;
    }
    return request;
}

BlacklistItem toBlacklistItem(const BlacklistItemPayload& payload) {
    BlacklistItem item;
    item.id = payload.id;
    item.user_id = payload.user_id;
    item.blocked_user_id = payload.blocked_user_id;
    item.created_at_ms = parseTimestampMs(payload.created_at);

    if (payload.blocked_user_info.has_value()) {
        item.blocked_user_info = toUserInfo(*payload.blocked_user_info);
    }
    if (item.blocked_user_info.user_id.empty()) {
        item.blocked_user_info.user_id = item.blocked_user_id;
    }
    return item;
}

const std::vector<FriendPayload>* toFriendPayloadList(const FriendListDataPayload& data) {
    return pickList(data.friends);
}

const std::vector<FriendRequestPayload>* toFriendRequestPayloadList(const FriendRequestListDataPayload& data) {
    return pickList(data.requests);
}

const std::vector<BlacklistItemPayload>* toBlacklistPayloadList(const BlacklistListDataPayload& data) {
    return pickList(data.items);
}

Friend parseNotificationFriend(const NotificationFriendEventPayload& payload) {
    Friend f;
    if (!payload.user_id.empty()) {
        f.user_id = payload.user_id;
    } else if (!payload.friend_user_id.empty()) {
        f.user_id = payload.friend_user_id;
    } else if (!payload.added_by_user_id.empty()) {
        f.user_id = payload.added_by_user_id;
    } else {
        f.user_id = payload.target_user_id;
    }

    f.remark = payload.remark;
    f.updated_at_ms = parseTimestampMs(payload.updated_at);
    f.is_deleted = payload.is_deleted;

    if (payload.user_info.has_value()) {
        f.user_info = toUserInfo(*payload.user_info);
    }
    if (f.user_info.user_id.empty()) {
        f.user_info.user_id = f.user_id;
    }
    return f;
}

FriendRequest parseNotificationFriendRequest(const NotificationFriendEventPayload& payload) {
    FriendRequest request;
    request.request_id = payload.request_id;
    request.from_user_id = payload.from_user_id;
    request.to_user_id = payload.to_user_id;
    request.message = payload.message;
    request.source = normalizeSource(payload.source);
    request.status = normalizeStatus(payload.status);
    request.created_at_ms = parseTimestampMs(payload.created_at);

    if (payload.from_user_info.has_value()) {
        request.from_user_info = toUserInfo(*payload.from_user_info);
    }
    if (request.from_user_info.user_id.empty()) {
        request.from_user_info.user_id = request.from_user_id;
    }
    return request;
}

BlacklistItem parseNotificationBlacklistItem(const NotificationFriendEventPayload& payload) {
    BlacklistItem item;
    item.id = payload.request_id;
    item.user_id = payload.user_id;
    item.blocked_user_id = payload.target_user_id;
    item.created_at_ms = parseTimestampMs(payload.changed_at);

    if (payload.blocked_user_info.has_value()) {
        item.blocked_user_info = toUserInfo(*payload.blocked_user_info);
    }
    if (item.blocked_user_info.user_id.empty()) {
        item.blocked_user_info.user_id = item.blocked_user_id;
    }
    return item;
}

std::string parseBlacklistAction(const NotificationFriendEventPayload& payload) {
    return payload.action;
}

int32_t parseRequestStatus(const NotificationFriendEventPayload& payload) {
    return normalizeStatus(payload.status);
}

bool isAddAction(const std::string& action) {
    const std::string lowered = toLowerCopy(action);
    return lowered == "add" || lowered == "added" || lowered == "create" || lowered == "created" || lowered == "block"
           || lowered == "blocked";
}

bool isRemoveAction(const std::string& action) {
    const std::string lowered = toLowerCopy(action);
    return lowered == "remove" || lowered == "removed" || lowered == "delete" || lowered == "deleted"
           || lowered == "unblock" || lowered == "unblocked";
}

void completeRequest(AnyChatCallback cb, network::HttpResponse resp, const std::string& fallback_error) {
    if (!cb.on_success && !cb.on_error) {
        return;
    }

    ApiEnvelope<void> root{};
    const bool ok = parseApiEnvelopeResponse(resp, root, fallback_error);
    if (ok) {
        if (cb.on_success) {
            cb.on_success();
        }
        return;
    }
    if (cb.on_error) {
        cb.on_error(root.code, root.message);
    }
}

} // namespace anychat::friend_manager_detail

namespace anychat {
using namespace friend_manager_detail;

FriendManagerImpl::FriendManagerImpl(
    db::Database* db,
    NotificationManager* notif_mgr,
    std::shared_ptr<network::HttpClient> http
)
    : db_(db)
    , notif_mgr_(notif_mgr)
    , http_(std::move(http)) {
    if (notif_mgr_) {
        notif_mgr_->addNotificationHandler([this](const NotificationEvent& ev) {
            handleFriendNotification(ev);
        });
    }
}

void FriendManagerImpl::getFriendList(AnyChatValueCallback<std::vector<Friend>> cb) {
    http_->get("/friends", [cb = std::move(cb), this](network::HttpResponse resp) {
        ApiEnvelope<FriendListDataPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        std::vector<Friend> friends;
        const auto* payloads = toFriendPayloadList(root.data);
        if (payloads != nullptr) {
            friends.reserve(payloads->size());
            for (const auto& item : *payloads) {
                Friend f = toFriend(item);
                friends.push_back(f);

                if (db_) {
                    db_->exec(
                        "INSERT OR REPLACE INTO friends"
                        " (user_id, remark, updated_at_ms, is_deleted,"
                        "  friend_nickname, friend_avatar)"
                        " VALUES (?, ?, ?, ?, ?, ?)",
                        { f.user_id,
                          f.remark,
                          f.updated_at_ms,
                          f.is_deleted ? int64_t{ 1 } : int64_t{ 0 },
                          f.user_info.username,
                          f.user_info.avatar_url },
                        nullptr
                    );
                }
            }
        }

        if (cb.on_success) {
            cb.on_success(friends);
        }
    });
}

void FriendManagerImpl::addFriend(
    const std::string& to_user_id,
    const std::string& message,
    int32_t source,
    AnyChatCallback cb
) {
    const SendFriendRequestBody body{
        .user_id = to_user_id,
        .message = message,
        .source = source > 0 ? source : kFriendSourceSearch,
    };

    std::string body_json;
    std::string err;
    if (!writeJson(body, body_json, err)) {
        if (cb.on_error) {
            cb.on_error(-1, err);
        }
        return;
    }

    http_->post("/friends/requests", body_json, [cb = std::move(cb)](network::HttpResponse resp) {
        completeRequest(std::move(cb), std::move(resp), "send request failed");
    });
}

void FriendManagerImpl::deleteFriend(const std::string& friend_id, AnyChatCallback cb) {
    const std::string path = "/friends/" + friend_id;
    http_->del(path, [cb = std::move(cb)](network::HttpResponse resp) {
        completeRequest(std::move(cb), std::move(resp), "delete friend failed");
    });
}

void FriendManagerImpl::updateRemark(const std::string& friend_id, const std::string& remark, AnyChatCallback cb) {
    const UpdateRemarkBody body{ .remark = remark };

    std::string body_json;
    std::string err;
    if (!writeJson(body, body_json, err)) {
        if (cb.on_error) {
            cb.on_error(-1, err);
        }
        return;
    }

    const std::string path = "/friends/" + friend_id + "/remark";
    http_->put(path, body_json, [cb = std::move(cb)](network::HttpResponse resp) {
        completeRequest(std::move(cb), std::move(resp), "update remark failed");
    });
}

void FriendManagerImpl::handleFriendRequest(int64_t request_id, int32_t action, AnyChatCallback cb) {
    if (action != kFriendRequestActionAccept && action != kFriendRequestActionReject) {
        if (cb.on_error) {
            cb.on_error(-1, "invalid action");
        }
        return;
    }

    const HandleFriendRequestBody body{ .action = action };

    std::string body_json;
    std::string err;
    if (!writeJson(body, body_json, err)) {
        if (cb.on_error) {
            cb.on_error(-1, err);
        }
        return;
    }

    const std::string path = "/friends/requests/" + std::to_string(request_id);
    http_->put(path, body_json, [cb = std::move(cb)](network::HttpResponse resp) {
        completeRequest(std::move(cb), std::move(resp), "handle request failed");
    });
}

void FriendManagerImpl::getFriendRequests(
    int32_t request_type,
    AnyChatValueCallback<std::vector<FriendRequest>> cb
) {
    int32_t type = request_type;
    if (type == 0) {
        type = kFriendRequestQueryTypeReceived;
    }
    if (type != kFriendRequestQueryTypeReceived && type != kFriendRequestQueryTypeSent) {
        if (cb.on_error) {
            cb.on_error(-1, "invalid request_type");
        }
        return;
    }

    const std::string path = "/friends/requests?request_type=" + std::to_string(type);

    http_->get(path, [cb = std::move(cb)](network::HttpResponse resp) {
        ApiEnvelope<FriendRequestListDataPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        std::vector<FriendRequest> requests;
        const auto* payloads = toFriendRequestPayloadList(root.data);
        if (payloads != nullptr) {
            requests.reserve(payloads->size());
            for (const auto& item : *payloads) {
                requests.push_back(toFriendRequest(item));
            }
        }

        if (cb.on_success) {
            cb.on_success(requests);
        }
    });
}

void FriendManagerImpl::getBlacklist(AnyChatValueCallback<std::vector<BlacklistItem>> cb) {
    http_->get("/friends/blacklist", [cb = std::move(cb)](network::HttpResponse resp) {
        ApiEnvelope<BlacklistListDataPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        std::vector<BlacklistItem> list;
        const auto* payloads = toBlacklistPayloadList(root.data);
        if (payloads != nullptr) {
            list.reserve(payloads->size());
            for (const auto& item : *payloads) {
                list.push_back(toBlacklistItem(item));
            }
        }

        if (cb.on_success) {
            cb.on_success(list);
        }
    });
}

void FriendManagerImpl::addToBlacklist(const std::string& user_id, AnyChatCallback cb) {
    const AddBlacklistBody body{ .user_id = user_id };

    std::string body_json;
    std::string err;
    if (!writeJson(body, body_json, err)) {
        if (cb.on_error) {
            cb.on_error(-1, err);
        }
        return;
    }

    http_->post("/friends/blacklist", body_json, [cb = std::move(cb)](network::HttpResponse resp) {
        completeRequest(std::move(cb), std::move(resp), "add blacklist failed");
    });
}

void FriendManagerImpl::removeFromBlacklist(const std::string& user_id, AnyChatCallback cb) {
    const std::string path = "/friends/blacklist/" + user_id;
    http_->del(path, [cb = std::move(cb)](network::HttpResponse resp) {
        completeRequest(std::move(cb), std::move(resp), "remove blacklist failed");
    });
}

void FriendManagerImpl::setListener(std::shared_ptr<FriendListener> listener) {
    std::lock_guard<std::mutex> lock(handler_mutex_);
    listener_ = std::move(listener);
}

void FriendManagerImpl::handleFriendNotification(const NotificationEvent& event) {
    const std::string& type = event.notification_type;
    if (type.rfind("friend.", 0) != 0) {
        return;
    }

    std::shared_ptr<FriendListener> listener;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        listener = listener_;
    }
    if (!listener) {
        return;
    }

    try {
        NotificationFriendEventPayload payload{};
        std::string err;
        if (!parseJsonObject(event.data, payload, err)) {
            return;
        }

        if (type == "friend.added") {
            listener->onFriendAdded(parseNotificationFriend(payload));
            return;
        }

        if (type == "friend.deleted") {
            listener->onFriendDeleted(payload.friend_user_id);
            return;
        }

        if (type == "friend.remark_updated") {
            listener->onFriendInfoChanged(parseNotificationFriend(payload));
            return;
        }

        if (type == "friend.blacklist_changed") {
            const std::string action = parseBlacklistAction(payload);
            if (isAddAction(action)) {
                listener->onBlacklistAdded(parseNotificationBlacklistItem(payload));
            } else if (isRemoveAction(action)) {
                listener->onBlacklistRemoved(payload.target_user_id);
            }
            return;
        }

        if (type == "friend.request") {
            listener->onFriendRequestReceived(parseNotificationFriendRequest(payload));
            return;
        }

        if (type == "friend.request_handled") {
            FriendRequest req = parseNotificationFriendRequest(payload);
            const int32_t status = parseRequestStatus(payload);
            if (status == kFriendRequestStatusAccepted) {
                listener->onFriendRequestAccepted(req);
            } else if (status == kFriendRequestStatusRejected) {
                listener->onFriendRequestRejected(req);
            } else if (status == kFriendRequestStatusExpired) {
                listener->onFriendRequestDeleted(req);
            }
            return;
        }
    } catch (...) {
    }
}

} // namespace anychat
