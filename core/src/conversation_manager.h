#pragma once

#include "notification_manager.h"
#include "sdk_callbacks.h"
#include "sdk_types.h"

#include "cache/conversation_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace anychat {

class ConversationListener {
public:
    virtual ~ConversationListener() = default;

    virtual void onConversationUpdated(const Conversation& conv) {
        (void) conv;
    }
};

class ConversationManagerImpl {
public:
    ConversationManagerImpl(
        db::Database* db,
        cache::ConversationCache* conv_cache,
        NotificationManager* notif_mgr,
        std::shared_ptr<network::HttpClient> http
    );

    // Returns cached + DB sorted list (pinned first, then by last_msg_time desc)
    void getConversationList(AnyChatValueCallback<std::vector<Conversation>> cb);

    // GET /conversations/{id}
    void getConversation(const std::string& conv_id, AnyChatValueCallback<Conversation> cb);

    // Delete conversation (local + DELETE /conversations/{id})
    void deleteConversation(const std::string& conv_id, AnyChatCallback cb);

    // POST /conversations/{id}/read-all
    void markAllRead(const std::string& conv_id, AnyChatCallback cb);

    // POST /conversations/{id}/messages/read
    void markMessagesRead(
        const std::string& conv_id,
        const std::vector<std::string>& message_ids,
        AnyChatValueCallback<ConversationMarkReadResult> cb
    );

    // Toggle pinned (local + PUT /conversations/{id}/pin)
    void setPinned(const std::string& conv_id, bool pinned, AnyChatCallback cb);

    // Toggle muted (local + PUT /conversations/{id}/mute)
    void setMuted(const std::string& conv_id, bool muted, AnyChatCallback cb);

    // PUT /conversations/{id}/burn
    void setBurnAfterReading(const std::string& conv_id, int32_t duration, AnyChatCallback cb);

    // PUT /conversations/{id}/auto_delete
    void setAutoDelete(const std::string& conv_id, int32_t duration, AnyChatCallback cb);

    // GET /conversations/unread/total
    void getTotalUnread(AnyChatValueCallback<int32_t> cb);

    // GET /conversations/{id}/messages/unread-count
    // last_read_seq < 0 means "not provided".
    void getMessageUnreadCount(const std::string& conv_id, int64_t last_read_seq, AnyChatValueCallback<ConversationUnreadState> cb);

    void getMessageUnreadCount(const std::string& conv_id, AnyChatValueCallback<ConversationUnreadState> cb) {
        getMessageUnreadCount(conv_id, -1, std::move(cb));
    }

    // GET /conversations/{id}/messages/read-receipts
    void getMessageReadReceipts(const std::string& conv_id, AnyChatValueCallback<std::vector<ConversationReadReceipt>> cb);

    // GET /conversations/{id}/messages/sequence
    void getMessageSequence(const std::string& conv_id, AnyChatValueCallback<int64_t> cb);

    // Listener fired whenever a conversation is updated (new message, read, etc.)
    void setListener(std::shared_ptr<ConversationListener> listener);

private:
    // Persist a conversation to the DB (upsert).
    void upsertDb(const Conversation& conv);

    // Populate a Conversation from a DB row.
    static Conversation rowToConversation(const db::Row& row);

    // Notification handler for conversation-related events.
    void handleConversationNotification(const NotificationEvent& event);

    db::Database* db_;
    cache::ConversationCache* conv_cache_;
    NotificationManager* notif_mgr_;
    std::shared_ptr<network::HttpClient> http_;

    mutable std::mutex handler_mutex_;
    std::shared_ptr<ConversationListener> listener_;
};

} // namespace anychat
