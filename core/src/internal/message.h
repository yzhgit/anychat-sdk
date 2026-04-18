#pragma once

#include "callbacks.h"
#include "types.h"

#include <memory>
#include <string>
#include <vector>

namespace anychat {

class MessageListener {
public:
    virtual ~MessageListener() = default;

    virtual void onMessageReceived(const Message& message) {
        (void) message;
    }

    virtual void onMessageReadReceipt(const MessageReadReceiptEvent& event) {
        (void) event;
    }

    virtual void onMessageRecalled(const Message& message) {
        (void) message;
    }

    virtual void onMessageDeleted(const Message& message) {
        (void) message;
    }

    virtual void onMessageEdited(const Message& message) {
        (void) message;
    }

    virtual void onMessageTyping(const MessageTypingEvent& event) {
        (void) event;
    }

    virtual void onMessageMentioned(const Message& message) {
        (void) message;
    }
};

class MessageManager {
public:
    virtual ~MessageManager() = default;

    virtual void
    sendTextMessage(const std::string& conv_id, const std::string& content, AnyChatCallback callback) = 0;

    virtual void
    getHistory(
        const std::string& conv_id,
        int64_t before_timestamp,
        int limit,
        AnyChatValueCallback<std::vector<Message>> callback
    ) = 0;

    virtual void markAsRead(const std::string& conv_id, const std::string& message_id, AnyChatCallback callback) = 0;

    // GET /messages/offline?lastSeq={last_seq}&limit={limit}
    virtual void getOfflineMessages(int64_t last_seq, int limit, AnyChatValueCallback<MessageOfflineResult> callback) = 0;

    // POST /messages/ack
    virtual void ackMessages(
        const std::string& conv_id,
        const std::vector<std::string>& message_ids,
        AnyChatCallback callback
    ) = 0;

    // GET /groups/{id}/messages/{msgId}/reads
    virtual void getGroupMessageReadState(
        const std::string& group_id,
        const std::string& message_id,
        AnyChatValueCallback<GroupMessageReadState> callback
    ) = 0;

    // GET /messages/search
    virtual void searchMessages(
        const std::string& keyword,
        const std::string& conversation_id,
        int32_t content_type,
        int limit,
        int offset,
        AnyChatValueCallback<MessageSearchResult> callback
    ) = 0;

    // Message operations (aligns with ws/http capabilities).
    virtual void recallMessage(const std::string& message_id, AnyChatCallback callback) = 0;
    virtual void deleteMessage(const std::string& message_id, AnyChatCallback callback) = 0;
    virtual void editMessage(const std::string& message_id, const std::string& content, AnyChatCallback callback) = 0;
    virtual void
    sendTyping(const std::string& conversation_id, bool typing, int32_t ttl_seconds, AnyChatCallback callback) = 0;

    virtual void setListener(std::shared_ptr<MessageListener> listener) = 0;
};

} // namespace anychat
