#include "notification_manager.h"

#include <cctype>
#include <initializer_list>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace anychat {
namespace {

const nlohmann::json* findField(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    if (!obj.is_object()) {
        return nullptr;
    }

    for (const char* key : keys) {
        auto it = obj.find(key);
        if (it != obj.end()) {
            return &(*it);
        }
    }
    return nullptr;
}

std::string snakeToCamel(const std::string& key) {
    std::string out;
    out.reserve(key.size());

    bool upper_next = false;
    for (unsigned char ch : key) {
        if (ch == '_') {
            upper_next = true;
            continue;
        }
        if (upper_next) {
            out.push_back(static_cast<char>(std::toupper(ch)));
            upper_next = false;
            continue;
        }
        out.push_back(static_cast<char>(ch));
    }
    return out;
}

void addCommonAliases(nlohmann::json& obj, const std::string& key, const nlohmann::json& value) {
    if (key == "fromUserId") {
        if (!obj.contains("senderId")) {
            obj["senderId"] = value;
        }
        return;
    }
    if (key == "seq") {
        if (!obj.contains("sequence")) {
            obj["sequence"] = value;
        }
        return;
    }
    if (key == "sentAt") {
        if (!obj.contains("timestamp")) {
            obj["timestamp"] = value;
        }
        return;
    }
    if (key == "groupName") {
        if (!obj.contains("name")) {
            obj["name"] = value;
        }
        return;
    }
    if (key == "groupAvatar") {
        if (!obj.contains("avatarUrl")) {
            obj["avatarUrl"] = value;
        }
        if (!obj.contains("avatar")) {
            obj["avatar"] = value;
        }
        return;
    }
    if (key == "inviterUserId") {
        if (!obj.contains("inviterId")) {
            obj["inviterId"] = value;
        }
        return;
    }
    if (key == "friendUserId" && !obj.contains("userId")) {
        obj["userId"] = value;
    }
}

nlohmann::json normalizeKeys(const nlohmann::json& value) {
    if (value.is_object()) {
        nlohmann::json normalized = nlohmann::json::object();
        for (auto it = value.begin(); it != value.end(); ++it) {
            const std::string key = snakeToCamel(it.key());
            nlohmann::json normalized_value = normalizeKeys(it.value());
            normalized[key] = normalized_value;
            addCommonAliases(normalized, key, normalized_value);
        }
        return normalized;
    }

    if (value.is_array()) {
        nlohmann::json normalized = nlohmann::json::array();
        for (const auto& item : value) {
            normalized.push_back(normalizeKeys(item));
        }
        return normalized;
    }

    return value;
}

std::string getString(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    const auto* value = findField(obj, keys);
    return value != nullptr && value->is_string() ? value->get<std::string>() : "";
}

int64_t getInt64(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    const auto* value = findField(obj, keys);
    if (value == nullptr) {
        return 0;
    }
    if (value->is_number_integer()) {
        return value->get<int64_t>();
    }
    if (value->is_number_unsigned()) {
        return static_cast<int64_t>(value->get<uint64_t>());
    }
    return 0;
}

nlohmann::json getObject(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    const auto* value = findField(obj, keys);
    if (value == nullptr || !value->is_object()) {
        return nlohmann::json::object();
    }
    return normalizeKeys(*value);
}

} // namespace

// ---------------------------------------------------------------------------
// Handler registration
// ---------------------------------------------------------------------------

void NotificationManager::setOnMessageSent(MsgSentHandler h) {
    std::unique_lock lock(mu_);
    on_msg_sent_ = std::move(h);
}

void NotificationManager::addNotificationHandler(NotifHandler h) {
    std::unique_lock lock(mu_);
    notification_handlers_.push_back(std::move(h));
}

void NotificationManager::setOnPong(PongHandler h) {
    std::unique_lock lock(mu_);
    on_pong_ = std::move(h);
}

// ---------------------------------------------------------------------------
// Frame dispatch
// ---------------------------------------------------------------------------

void NotificationManager::handleRaw(const std::string& raw_json) {
    // Parse — silently discard any frame that is not valid JSON or lacks "type".
    nlohmann::json frame;
    try {
        frame = nlohmann::json::parse(raw_json);
    } catch (const nlohmann::json::exception&) {
        return;
    }

    if (!frame.is_object() || !frame.contains("type") || !frame["type"].is_string()) {
        return;
    }

    const std::string type = frame["type"].get<std::string>();

    // ---- pong ---------------------------------------------------------------
    if (type == "pong") {
        PongHandler handler;
        {
            std::shared_lock lock(mu_);
            handler = on_pong_;
        }
        if (handler) {
            handler();
        }
        return;
    }

    // ---- message.sent -------------------------------------------------------
    if (type == "message.sent") {
        if (!frame.contains("payload") || !frame["payload"].is_object()) {
            return;
        }
        const auto& p = frame["payload"];

        MsgSentAck ack;
        ack.message_id = getString(p, { "messageId", "message_id" });
        ack.sequence = getInt64(p, { "sequence" });
        ack.timestamp = getInt64(p, { "timestamp" });
        ack.local_id = getString(p, { "localId", "local_id" });

        MsgSentHandler handler;
        {
            std::shared_lock lock(mu_);
            handler = on_msg_sent_;
        }
        if (handler) {
            handler(ack);
        }
        return;
    }

    // ---- notification -------------------------------------------------------
    if (type == "notification") {
        if (!frame.contains("payload") || !frame["payload"].is_object()) {
            return;
        }
        const auto& p = frame["payload"];

        NotificationEvent evt;
        evt.notification_type = getString(p, { "notificationType", "type" });
        evt.timestamp = getInt64(p, { "timestamp" });
        evt.data = getObject(p, { "data", "payload" });

        std::vector<NotifHandler> handlers;
        {
            std::shared_lock lock(mu_);
            handlers = notification_handlers_;
        }
        for (const auto& h : handlers) {
            if (h) {
                h(evt);
            }
        }
        return;
    }

    // Unknown type — silently ignore.
}

} // namespace anychat
