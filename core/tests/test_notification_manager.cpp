#include "notification_manager.h"

#include <atomic>
#include <string>

#include <gtest/gtest.h>

// The NotificationManager dispatches on the calling thread (handleRaw() is
// synchronous), so no threading or async machinery is needed in these tests.

// ===========================================================================
// NotificationManager tests
// ===========================================================================

TEST(NotificationManagerTest, PongDispatch) {
    anychat::NotificationManager mgr;

    int call_count = 0;
    mgr.setOnPong([&call_count]() {
        ++call_count;
    });

    mgr.handleRaw(R"({"type":"pong"})");

    EXPECT_EQ(call_count, 1) << "Pong handler should be called exactly once";
}

TEST(NotificationManagerTest, MessageSentDispatch) {
    anychat::NotificationManager mgr;

    anychat::MsgSentAck received_ack{};
    int call_count = 0;

    mgr.setOnMessageSent([&](const anychat::MsgSentAck& ack) {
        received_ack = ack;
        ++call_count;
    });

    const std::string frame = R"({
        "type": "message.sent",
        "payload": {
            "messageId": "msg-server-001",
            "sequence": 42,
            "timestamp": 1708329600,
            "localId": "local-uuid-99"
        }
    })";

    mgr.handleRaw(frame);

    ASSERT_EQ(call_count, 1) << "MessageSent handler should be called once";
    EXPECT_EQ(received_ack.message_id, "msg-server-001");
    EXPECT_EQ(received_ack.sequence, 42);
    EXPECT_EQ(received_ack.timestamp, 1708329600);
    EXPECT_EQ(received_ack.local_id, "local-uuid-99");
}

TEST(NotificationManagerTest, NotificationDispatch) {
    anychat::NotificationManager mgr;

    anychat::NotificationEvent received_event{};
    int call_count = 0;

    mgr.addNotificationHandler([&](const anychat::NotificationEvent& event) {
        received_event = event;
        ++call_count;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-001",
            "type": "message.new",
            "timestamp": 1708329600,
            "payload": {
                "message_id": "msg-111",
                "conversation_id": "conv-222",
                "from_user_id": "user-333",
                "content_type": "text",
                "content": "你好吗？",
                "sent_at": 1708329595,
                "seq": 43
            }
        }
    })";

    mgr.handleRaw(frame);

    ASSERT_EQ(call_count, 1) << "Notification handler should be called once";
    EXPECT_EQ(received_event.notification_type, "message.new");
    EXPECT_EQ(received_event.timestamp, 1708329600);
    EXPECT_EQ(received_event.data.value("messageId", ""), "msg-111");
    EXPECT_EQ(received_event.data.value("conversationId", ""), "conv-222");
    EXPECT_EQ(received_event.data.value("fromUserId", ""), "user-333");
    EXPECT_EQ(received_event.data.value("senderId", ""), "user-333");
    EXPECT_EQ(received_event.data.value("contentType", ""), "text");
    EXPECT_EQ(received_event.data.value("sequence", int64_t{ 0 }), 43);
    EXPECT_EQ(received_event.data.value("timestamp", int64_t{ 0 }), 1708329595);
}

TEST(NotificationManagerTest, UnknownTypeIgnored) {
    anychat::NotificationManager mgr;

    bool any_called = false;
    mgr.setOnPong([&]() {
        any_called = true;
    });
    mgr.setOnMessageSent([&](const anychat::MsgSentAck&) {
        any_called = true;
    });
    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) {
        any_called = true;
    });

    EXPECT_NO_THROW(mgr.handleRaw(R"({"type":"unknown_event"})"));
    EXPECT_FALSE(any_called) << "No handler should be called for an unknown frame type";
}

TEST(NotificationManagerTest, MalformedJsonIgnored) {
    anychat::NotificationManager mgr;

    bool any_called = false;
    mgr.setOnPong([&]() {
        any_called = true;
    });
    mgr.setOnMessageSent([&](const anychat::MsgSentAck&) {
        any_called = true;
    });
    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) {
        any_called = true;
    });

    EXPECT_NO_THROW(mgr.handleRaw("not json at all {{{{"));
    EXPECT_NO_THROW(mgr.handleRaw(""));
    EXPECT_NO_THROW(mgr.handleRaw("{"));

    EXPECT_FALSE(any_called) << "No handler should be called for malformed JSON";
}

TEST(NotificationManagerTest, HandlerCanBeCleared) {
    anychat::NotificationManager mgr;

    int count = 0;
    mgr.setOnPong([&count]() {
        ++count;
    });

    mgr.handleRaw(R"({"type":"pong"})");
    EXPECT_EQ(count, 1);

    mgr.setOnPong(nullptr);

    EXPECT_NO_THROW(mgr.handleRaw(R"({"type":"pong"})"));
    EXPECT_EQ(count, 1) << "Cleared handler should not be invoked";
}

TEST(NotificationManagerTest, FriendRequestNotificationDispatch) {
    anychat::NotificationManager mgr;

    std::string received_type;
    mgr.addNotificationHandler([&](const anychat::NotificationEvent& e) {
        received_type = e.notification_type;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-002",
            "type": "friend.request",
            "timestamp": 1708329601,
            "payload": {
                "request_id": 444,
                "from_user_id": "user-555",
                "message": "你好，我是张三"
            }
        }
    })";

    mgr.handleRaw(frame);
    EXPECT_EQ(received_type, "friend.request");
}

TEST(NotificationManagerTest, MultipleHandlersFanOut) {
    anychat::NotificationManager mgr;

    int count_a = 0;
    int count_b = 0;

    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) {
        ++count_a;
    });
    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) {
        ++count_b;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-003",
            "type": "message.new",
            "timestamp": 1708329600,
            "payload": {}
        }
    })";

    mgr.handleRaw(frame);

    EXPECT_EQ(count_a, 1) << "First handler should be called";
    EXPECT_EQ(count_b, 1) << "Second handler should also be called";

    mgr.handleRaw(frame);
    EXPECT_EQ(count_a, 2);
    EXPECT_EQ(count_b, 2);
}
