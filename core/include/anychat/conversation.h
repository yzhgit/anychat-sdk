#pragma once

#include "callbacks.h"
#include "types.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace anychat {

class ConversationListener {
public:
    virtual ~ConversationListener() = default;

    virtual void onConversationUpdated(const Conversation& conv) {
        (void) conv;
    }
};

class ConversationManager {
public:
    virtual ~ConversationManager() = default;

    // Returns cached + DB sorted list (pinned first, then by last_msg_time desc)
    virtual void getConversationList(AnyChatValueCallback<std::vector<Conversation>> cb) = 0;

    // GET /conversations/{id}
    virtual void getConversation(const std::string& conv_id, AnyChatValueCallback<Conversation> cb) = 0;

    // Delete conversation (local + DELETE /conversations/{id})
    virtual void deleteConversation(const std::string& conv_id, AnyChatCallback cb) = 0;

    // POST /conversations/{id}/read-all
    virtual void markAllRead(const std::string& conv_id, AnyChatCallback cb) = 0;

    // POST /conversations/{id}/messages/read
    virtual void markMessagesRead(
        const std::string& conv_id,
        const std::vector<std::string>& message_ids,
        AnyChatValueCallback<ConversationMarkReadResult> cb
    ) = 0;

    // Toggle pinned (local + PUT /conversations/{id}/pin)
    virtual void setPinned(const std::string& conv_id, bool pinned, AnyChatCallback cb) = 0;

    // Toggle muted (local + PUT /conversations/{id}/mute)
    virtual void setMuted(const std::string& conv_id, bool muted, AnyChatCallback cb) = 0;

    // PUT /conversations/{id}/burn
    virtual void setBurnAfterReading(const std::string& conv_id, int32_t duration, AnyChatCallback cb) = 0;

    // PUT /conversations/{id}/auto_delete
    virtual void setAutoDelete(const std::string& conv_id, int32_t duration, AnyChatCallback cb) = 0;

    // GET /conversations/unread/total
    virtual void getTotalUnread(AnyChatValueCallback<int32_t> cb) = 0;

    // GET /conversations/{id}/messages/unread-count
    // last_read_seq < 0 means "not provided".
    virtual void
    getMessageUnreadCount(const std::string& conv_id, int64_t last_read_seq, AnyChatValueCallback<ConversationUnreadState> cb) = 0;

    virtual void getMessageUnreadCount(const std::string& conv_id, AnyChatValueCallback<ConversationUnreadState> cb) {
        getMessageUnreadCount(conv_id, -1, std::move(cb));
    }

    // GET /conversations/{id}/messages/read-receipts
    virtual void getMessageReadReceipts(const std::string& conv_id, AnyChatValueCallback<std::vector<ConversationReadReceipt>> cb) = 0;

    // GET /conversations/{id}/messages/sequence
    virtual void getMessageSequence(const std::string& conv_id, AnyChatValueCallback<int64_t> cb) = 0;

    // Listener fired whenever a conversation is updated (new message, read, etc.)
    virtual void setListener(std::shared_ptr<ConversationListener> listener) = 0;
};

} // namespace anychat
