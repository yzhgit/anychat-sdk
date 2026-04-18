#pragma once

#include "notification_manager.h"
#include "outbound_queue.h"

#include "internal/message.h"

#include "cache/message_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <mutex>
#include <string>
#include <vector>

namespace anychat {

class MessageManagerImpl : public MessageManager {
public:
    MessageManagerImpl(
        db::Database* db,
        cache::MessageCache* msg_cache,
        OutboundQueue* outbound_q,
        NotificationManager* notif_mgr,
        std::shared_ptr<network::HttpClient> http,
        const std::string& current_user_id
    );

    void sendTextMessage(const std::string& conv_id, const std::string& content, AnyChatCallback callback) override;

    void
    getHistory(const std::string& conv_id, int64_t before_timestamp, int limit, AnyChatValueCallback<std::vector<Message>> callback)
        override;

    void markAsRead(const std::string& conv_id, const std::string& message_id, AnyChatCallback callback) override;
    void getOfflineMessages(int64_t last_seq, int limit, AnyChatValueCallback<MessageOfflineResult> callback) override;
    void ackMessages(
        const std::string& conv_id,
        const std::vector<std::string>& message_ids,
        AnyChatCallback callback
    ) override;
    void getGroupMessageReadState(
        const std::string& group_id,
        const std::string& message_id,
        AnyChatValueCallback<GroupMessageReadState> callback
    ) override;
    void searchMessages(
        const std::string& keyword,
        const std::string& conversation_id,
        int32_t content_type,
        int limit,
        int offset,
        AnyChatValueCallback<MessageSearchResult> callback
    ) override;
    void recallMessage(const std::string& message_id, AnyChatCallback callback) override;
    void deleteMessage(const std::string& message_id, AnyChatCallback callback) override;
    void editMessage(const std::string& message_id, const std::string& content, AnyChatCallback callback) override;
    void sendTyping(const std::string& conversation_id, bool typing, int32_t ttl_seconds, AnyChatCallback callback)
        override;

    void setListener(std::shared_ptr<MessageListener> listener) override;

    // Called by AnyChatClientImpl when current_user_id becomes known after login.
    void setCurrentUserId(const std::string& uid);

private:
    static std::string urlEncode(const std::string& input);
    static int64_t normalizeEpochMs(int64_t raw);
    void upsertMessageDb(const Message& msg);
    void updateMessageDbStatusAndContent(const std::string& message_id, int status, const std::string* content);

    void handleIncomingMessage(const NotificationEvent& event);
    void handleReadReceipt(const NotificationEvent& event);
    void handleMessageRecalled(const NotificationEvent& event);
    void handleMessageDeleted(const NotificationEvent& event);
    void handleMessageEdited(const NotificationEvent& event);
    void handleTyping(const NotificationEvent& event);
    void handleMentioned(const NotificationEvent& event);
    static std::string generateLocalId();

    db::Database* db_;
    cache::MessageCache* msg_cache_;
    OutboundQueue* outbound_q_;
    NotificationManager* notif_mgr_;
    std::shared_ptr<network::HttpClient> http_;

    mutable std::mutex handler_mutex_;
    std::shared_ptr<MessageListener> listener_;

    std::mutex uid_mutex_;
    std::string current_user_id_;
};

} // namespace anychat
