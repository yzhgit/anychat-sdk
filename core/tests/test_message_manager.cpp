#include <gtest/gtest.h>
#include "message_manager.h"
#include "notification_manager.h"
#include "outbound_queue.h"
#include "cache/message_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <atomic>
#include <memory>
#include <string>

// ===========================================================================
// Fixture
// ===========================================================================
class MessageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<anychat::db::Database>(":memory:");
        ASSERT_TRUE(db_->open()) << "Failed to open in-memory DB";

        msg_cache_  = std::make_unique<anychat::cache::MessageCache>();
        notif_mgr_  = std::make_unique<anychat::NotificationManager>();
        outbound_q_ = std::make_unique<anychat::OutboundQueue>(db_.get());

        // Use a dummy HTTP client — no real network is needed.
        http_ = std::make_shared<anychat::network::HttpClient>(
            "http://localhost:19999");

        // Construct the message manager — this registers the notification
        // handler on notif_mgr_ internally.
        mgr_ = std::make_unique<anychat::MessageManagerImpl>(
            db_.get(),
            msg_cache_.get(),
            outbound_q_.get(),
            notif_mgr_.get(),
            http_,
            "user-test-001");
    }

    void TearDown() override {
        mgr_.reset();
        outbound_q_.reset();
        notif_mgr_.reset();
        msg_cache_.reset();
        http_.reset();
        db_->close();
        db_.reset();
    }

    // Drain the DB worker queue (ensures pending async exec() calls complete).
    void drainDb() {
        db_->querySync("SELECT 1");
    }

    // Count rows in outbound_queue.
    int outboundRowCount() {
        return static_cast<int>(
            db_->querySync("SELECT local_id FROM outbound_queue").size());
    }

    std::unique_ptr<anychat::db::Database>          db_;
    std::unique_ptr<anychat::cache::MessageCache>   msg_cache_;
    std::unique_ptr<anychat::NotificationManager>   notif_mgr_;
    std::unique_ptr<anychat::OutboundQueue>         outbound_q_;
    std::shared_ptr<anychat::network::HttpClient>   http_;
    std::unique_ptr<anychat::MessageManagerImpl>    mgr_;
};

// ---------------------------------------------------------------------------
// 1. SendTextMessageEnqueues
//    sendTextMessage() should insert a row into outbound_queue in the DB.
//    The callback must NOT be called before an ack arrives.
// ---------------------------------------------------------------------------
TEST_F(MessageManagerTest, SendTextMessageEnqueues) {
    bool cb_called = false;

    mgr_->sendTextMessage("conv-1", "hello",
        [&cb_called](bool /*success*/, const std::string& /*err*/) {
            cb_called = true;
        });

    // Drain DB so the async INSERT is committed.
    drainDb();

    EXPECT_EQ(outboundRowCount(), 1)
        << "One row should be present in outbound_queue";
    EXPECT_FALSE(cb_called)
        << "Callback should not be called before the server ack";
}

// ---------------------------------------------------------------------------
// 2. IncomingMessageFiresHandler
//    Register an OnMessageReceived handler; simulate an incoming message.new
//    notification via NotificationManager. The handler should be invoked with
//    the correct Message fields.
// ---------------------------------------------------------------------------
TEST_F(MessageManagerTest, IncomingMessageFiresHandler) {
    anychat::Message received_msg{};
    int call_count = 0;

    mgr_->setOnMessageReceived([&](const anychat::Message& msg) {
        received_msg = msg;
        ++call_count;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "message.new",
            "timestamp": 1708329600,
            "data": {
                "messageId": "msg-incoming-001",
                "conversationId": "conv-1",
                "senderId": "user-sender-999",
                "contentType": "text",
                "content": "你好吗？",
                "sequence": 7
            }
        }
    })";

    notif_mgr_->handleRaw(frame);

    ASSERT_EQ(call_count, 1) << "OnMessageReceived handler should be called once";
    EXPECT_EQ(received_msg.message_id,   "msg-incoming-001");
    EXPECT_EQ(received_msg.conv_id,      "conv-1");
    EXPECT_EQ(received_msg.sender_id,    "user-sender-999");
    EXPECT_EQ(received_msg.content,      "你好吗？");
    EXPECT_EQ(received_msg.seq,          7);
    EXPECT_EQ(received_msg.timestamp_ms, 1708329600LL * 1000LL);
}

// ---------------------------------------------------------------------------
// 3. IncomingMessageCacheDedup
//    Inserting the same message_id twice must result in only one entry in the
//    message cache. (The user handler may be called for each raw notification,
//    but the cache must not store duplicates.)
// ---------------------------------------------------------------------------
TEST_F(MessageManagerTest, IncomingMessageCacheDedup) {
    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "message.new",
            "timestamp": 1708329600,
            "data": {
                "messageId": "msg-dup-001",
                "conversationId": "conv-dedup",
                "senderId": "user-A",
                "contentType": "text",
                "content": "duplicate",
                "sequence": 1
            }
        }
    })";

    // Fire the same notification twice.
    notif_mgr_->handleRaw(frame);
    notif_mgr_->handleRaw(frame);

    auto cached = msg_cache_->get("conv-dedup");
    EXPECT_EQ(cached.size(), 1u)
        << "Message cache should store only one copy of a duplicate message_id";
    EXPECT_EQ(cached[0].message_id, "msg-dup-001");
}

// ---------------------------------------------------------------------------
// 4. NonMessageNewNotificationDoesNotFireHandler
//    Notifications of a type other than "message.new" should not trigger the
//    OnMessageReceived handler.
// ---------------------------------------------------------------------------
TEST_F(MessageManagerTest, NonMessageNewNotificationDoesNotFireHandler) {
    int call_count = 0;
    mgr_->setOnMessageReceived([&](const anychat::Message&) { ++call_count; });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "friend.request",
            "timestamp": 1708329601,
            "data": {
                "requestId": "req-999",
                "fromUserId": "user-X"
            }
        }
    })";

    notif_mgr_->handleRaw(frame);

    EXPECT_EQ(call_count, 0)
        << "OnMessageReceived should not be called for non-message.new notifications";
}

// ---------------------------------------------------------------------------
// 5. SendTextMessageMultiple
//    Sending several messages enqueues each as a separate DB row.
// ---------------------------------------------------------------------------
TEST_F(MessageManagerTest, SendTextMessageMultiple) {
    mgr_->sendTextMessage("conv-m", "first",  nullptr);
    mgr_->sendTextMessage("conv-m", "second", nullptr);
    mgr_->sendTextMessage("conv-m", "third",  nullptr);

    drainDb();

    EXPECT_EQ(outboundRowCount(), 3)
        << "Each sendTextMessage call should create one outbound_queue row";
}

// ---------------------------------------------------------------------------
// 6. SetCurrentUserId
//    setCurrentUserId should not crash and the manager should continue to
//    work normally afterward.
// ---------------------------------------------------------------------------
TEST_F(MessageManagerTest, SetCurrentUserId) {
    EXPECT_NO_THROW(mgr_->setCurrentUserId("new-user-id"));

    bool handler_called = false;
    mgr_->setOnMessageReceived([&](const anychat::Message&) {
        handler_called = true;
    });

    notif_mgr_->handleRaw(R"({
        "type": "notification",
        "payload": {
            "notificationType": "message.new",
            "timestamp": 1708329700,
            "data": {
                "messageId": "msg-after-uid-change",
                "conversationId": "conv-x",
                "senderId": "other-user",
                "contentType": "text",
                "content": "hello",
                "sequence": 1
            }
        }
    })");

    EXPECT_TRUE(handler_called)
        << "Handler should still fire after setCurrentUserId";
}
