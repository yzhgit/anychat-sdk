#include "notification_manager.h"

#include <mutex>
#include <shared_mutex>

namespace anychat {

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
        if (p.contains("messageId") && p["messageId"].is_string()) {
            ack.message_id = p["messageId"].get<std::string>();
        }
        if (p.contains("sequence") && p["sequence"].is_number_integer()) {
            ack.sequence = p["sequence"].get<int64_t>();
        }
        if (p.contains("timestamp") && p["timestamp"].is_number_integer()) {
            ack.timestamp = p["timestamp"].get<int64_t>();
        }
        if (p.contains("localId") && p["localId"].is_string()) {
            ack.local_id = p["localId"].get<std::string>();
        }

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
        if (p.contains("notificationType") && p["notificationType"].is_string()) {
            evt.notification_type = p["notificationType"].get<std::string>();
        }
        if (p.contains("timestamp") && p["timestamp"].is_number_integer()) {
            evt.timestamp = p["timestamp"].get<int64_t>();
        }
        if (p.contains("data") && p["data"].is_object()) {
            evt.data = p["data"];
        }

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
