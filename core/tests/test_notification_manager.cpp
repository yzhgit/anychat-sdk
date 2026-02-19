#include <gtest/gtest.h>
#include "notification_manager.h"

#include <atomic>
#include <string>

// The NotificationManager dispatches on the calling thread (handleRaw() is
// synchronous), so no threading or async machinery is needed in these tests.

// ===========================================================================
// NotificationManager tests
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. PongDispatch
//    Registering a pong handler and calling handleRaw with {"type":"pong"}
//    should invoke the handler exactly once.
// ---------------------------------------------------------------------------
TEST(NotificationManagerTest, PongDispatch) {
    anychat::NotificationManager mgr;

    int call_count = 0;
    mgr.setOnPong([&call_count]() {
        ++call_count;
    });

    mgr.handleRaw(R"({"type":"pong"})");

    EXPECT_EQ(call_count, 1) << "Pong handler should be called exactly once";
}

// ---------------------------------------------------------------------------
// 2. MessageSentDispatch
//    Registering a message-sent handler and calling handleRaw with a valid
//    message.sent frame should invoke the handler with correct MsgSentAck data.
// ---------------------------------------------------------------------------
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
    EXPECT_EQ(received_ack.sequence,   42);
    EXPECT_EQ(received_ack.timestamp,  1708329600);
    EXPECT_EQ(received_ack.local_id,   "local-uuid-99");
}

// ---------------------------------------------------------------------------
// 3. NotificationDispatch
//    Registering a notification handler and calling handleRaw with a
//    notification frame should deliver the correct NotificationEvent.
// ---------------------------------------------------------------------------
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
            "notificationType": "message.new",
            "timestamp": 1708329600,
            "data": {
                "messageId": "msg-111",
                "conversationId": "conv-222",
                "senderId": "user-333",
                "contentType": "text",
                "content": "你好吗？",
                "sequence": 43
            }
        }
    })";

    mgr.handleRaw(frame);

    ASSERT_EQ(call_count, 1) << "Notification handler should be called once";
    EXPECT_EQ(received_event.notification_type, "message.new");
    EXPECT_EQ(received_event.timestamp, 1708329600);
    EXPECT_EQ(received_event.data.value("messageId", ""),       "msg-111");
    EXPECT_EQ(received_event.data.value("conversationId", ""),  "conv-222");
}

// ---------------------------------------------------------------------------
// 4. UnknownTypeIgnored
//    A frame with an unrecognized type should not invoke any handler and
//    should not crash.
// ---------------------------------------------------------------------------
TEST(NotificationManagerTest, UnknownTypeIgnored) {
    anychat::NotificationManager mgr;

    bool any_called = false;
    mgr.setOnPong([&]()                               { any_called = true; });
    mgr.setOnMessageSent([&](const anychat::MsgSentAck&) { any_called = true; });
    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) { any_called = true; });

    // Should be silently ignored.
    EXPECT_NO_THROW(mgr.handleRaw(R"({"type":"unknown_event"})"));
    EXPECT_FALSE(any_called) << "No handler should be called for an unknown frame type";
}

// ---------------------------------------------------------------------------
// 5. MalformedJsonIgnored
//    handleRaw() with invalid JSON should not crash and should not invoke
//    any handler.
// ---------------------------------------------------------------------------
TEST(NotificationManagerTest, MalformedJsonIgnored) {
    anychat::NotificationManager mgr;

    bool any_called = false;
    mgr.setOnPong([&]()                               { any_called = true; });
    mgr.setOnMessageSent([&](const anychat::MsgSentAck&) { any_called = true; });
    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) { any_called = true; });

    // Must not throw or crash.
    EXPECT_NO_THROW(mgr.handleRaw("not json at all {{{{"));
    EXPECT_NO_THROW(mgr.handleRaw(""));
    EXPECT_NO_THROW(mgr.handleRaw("{"));

    EXPECT_FALSE(any_called) << "No handler should be called for malformed JSON";
}

// ---------------------------------------------------------------------------
// 6. HandlerCanBeCleared
//    Passing nullptr to a setter should clear the handler; subsequent frames
//    of that type should not crash.
// ---------------------------------------------------------------------------
TEST(NotificationManagerTest, HandlerCanBeCleared) {
    anychat::NotificationManager mgr;

    int count = 0;
    mgr.setOnPong([&count]() { ++count; });

    mgr.handleRaw(R"({"type":"pong"})");
    EXPECT_EQ(count, 1);

    // Clear the pong handler.
    mgr.setOnPong(nullptr);

    // Second pong should not crash and should not increment count.
    EXPECT_NO_THROW(mgr.handleRaw(R"({"type":"pong"})"));
    EXPECT_EQ(count, 1) << "Cleared handler should not be invoked";
}

// ---------------------------------------------------------------------------
// 7. MultipleNotificationTypes
//    Verify that each notification type frame reaches the correct handler.
// ---------------------------------------------------------------------------
TEST(NotificationManagerTest, FriendRequestNotificationDispatch) {
    anychat::NotificationManager mgr;

    std::string received_type;
    mgr.addNotificationHandler([&](const anychat::NotificationEvent& e) {
        received_type = e.notification_type;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "friend.request",
            "timestamp": 1708329601,
            "data": {
                "requestId": "req-444",
                "fromUserId": "user-555",
                "message": "你好，我是张三"
            }
        }
    })";

    mgr.handleRaw(frame);
    EXPECT_EQ(received_type, "friend.request");
}

// ---------------------------------------------------------------------------
// 8. MultipleHandlersFanOut
//    Registering two notification handlers should cause both to be called for
//    every notification frame (fan-out semantics).
// ---------------------------------------------------------------------------
TEST(NotificationManagerTest, MultipleHandlersFanOut) {
    anychat::NotificationManager mgr;

    int count_a = 0;
    int count_b = 0;

    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) { ++count_a; });
    mgr.addNotificationHandler([&](const anychat::NotificationEvent&) { ++count_b; });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "message.new",
            "timestamp": 1708329600,
            "data": {}
        }
    })";

    mgr.handleRaw(frame);

    EXPECT_EQ(count_a, 1) << "First handler should be called";
    EXPECT_EQ(count_b, 1) << "Second handler should also be called";

    // A second frame should invoke both handlers again.
    mgr.handleRaw(frame);
    EXPECT_EQ(count_a, 2);
    EXPECT_EQ(count_b, 2);
}
