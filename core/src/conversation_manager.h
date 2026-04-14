#pragma once
#include "notification_manager.h"

#include "internal/conversation.h"

#include "cache/conversation_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <mutex>

namespace anychat {

class ConversationManagerImpl : public ConversationManager {
public:
    ConversationManagerImpl(
        db::Database* db,
        cache::ConversationCache* conv_cache,
        NotificationManager* notif_mgr,
        std::shared_ptr<network::HttpClient> http
    );

    // ConversationManager interface
    void getConversationList(AnyChatValueCallback<std::vector<Conversation>> cb) override;
    void getConversation(const std::string& conv_id, AnyChatValueCallback<Conversation> cb) override;
    void deleteConversation(const std::string& conv_id, AnyChatCallback cb) override;
    void markAllRead(const std::string& conv_id, AnyChatCallback cb) override;
    void markMessagesRead(
        const std::string& conv_id,
        const std::vector<std::string>& message_ids,
        AnyChatValueCallback<ConversationMarkReadResult> cb
    ) override;
    void setPinned(const std::string& conv_id, bool pinned, AnyChatCallback cb) override;
    void setMuted(const std::string& conv_id, bool muted, AnyChatCallback cb) override;
    void setBurnAfterReading(const std::string& conv_id, int32_t duration, AnyChatCallback cb) override;
    void setAutoDelete(const std::string& conv_id, int32_t duration, AnyChatCallback cb) override;
    void getTotalUnread(AnyChatValueCallback<int32_t> cb) override;
    void getMessageUnreadCount(
        const std::string& conv_id,
        int64_t last_read_seq,
        AnyChatValueCallback<ConversationUnreadState> cb
    )
        override;
    void getMessageReadReceipts(const std::string& conv_id, AnyChatValueCallback<std::vector<ConversationReadReceipt>> cb)
        override;
    void getMessageSequence(const std::string& conv_id, AnyChatValueCallback<int64_t> cb) override;
    void setListener(std::shared_ptr<ConversationListener> listener) override;

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
