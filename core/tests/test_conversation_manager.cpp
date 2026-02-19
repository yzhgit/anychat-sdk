#include <gtest/gtest.h>
#include "conversation_manager.h"
#include "notification_manager.h"
#include "cache/conversation_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <string>

// ===========================================================================
// Fixture
// ===========================================================================
class ConversationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<anychat::db::Database>(":memory:");
        ASSERT_TRUE(db_->open());

        conv_cache_ = std::make_unique<anychat::cache::ConversationCache>();
        notif_mgr_  = std::make_unique<anychat::NotificationManager>();

        http_ = std::make_shared<anychat::network::HttpClient>(
            "http://localhost:19999");

        mgr_ = std::make_unique<anychat::ConversationManagerImpl>(
            db_.get(), conv_cache_.get(), notif_mgr_.get(), http_);
    }

    void TearDown() override {
        mgr_.reset();
        http_.reset();
        notif_mgr_.reset();
        conv_cache_.reset();
        db_->close();
        db_.reset();
    }

    std::unique_ptr<anychat::db::Database>              db_;
    std::unique_ptr<anychat::cache::ConversationCache>  conv_cache_;
    std::unique_ptr<anychat::NotificationManager>       notif_mgr_;
    std::shared_ptr<anychat::network::HttpClient>       http_;
    std::unique_ptr<anychat::ConversationManagerImpl>   mgr_;
};

// ---------------------------------------------------------------------------
// 1. GetListEmptyInitially
//    Before any sync or notification, the conversation list is empty.
// ---------------------------------------------------------------------------
TEST_F(ConversationManagerTest, GetListEmptyInitially) {
    std::vector<anychat::Conversation> result;
    mgr_->getList([&](std::vector<anychat::Conversation> list, const std::string& /*err*/) {
        result = list;
    });
    EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// 2. SessionUnreadUpdatedNotificationFiresHandler
//    A session.unread_updated WebSocket notification should invoke the
//    OnConversationUpdated handler.
// ---------------------------------------------------------------------------
TEST_F(ConversationManagerTest, SessionUnreadUpdatedNotificationFiresHandler) {
    int call_count = 0;
    mgr_->setOnConversationUpdated([&](const anychat::Conversation&) {
        ++call_count;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "session.unread_updated",
            "timestamp": 1708329600,
            "data": {
                "sessionId": "conv-001",
                "unreadCount": 3
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_GE(call_count, 1)
        << "OnConversationUpdated should fire on session.unread_updated";
}

// ---------------------------------------------------------------------------
// 3. SessionDeletedNotificationFiresHandler
//    A session.deleted notification should invoke the OnConversationUpdated
//    handler.
// ---------------------------------------------------------------------------
TEST_F(ConversationManagerTest, SessionDeletedNotificationFiresHandler) {
    int call_count = 0;
    mgr_->setOnConversationUpdated([&](const anychat::Conversation&) {
        ++call_count;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "session.deleted",
            "timestamp": 1708329601,
            "data": {
                "sessionId": "conv-002"
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_GE(call_count, 1);
}

// ---------------------------------------------------------------------------
// 4. UnrelatedNotificationDoesNotFireHandler
//    Notifications unrelated to sessions must not call OnConversationUpdated.
// ---------------------------------------------------------------------------
TEST_F(ConversationManagerTest, UnrelatedNotificationDoesNotFireHandler) {
    int call_count = 0;
    mgr_->setOnConversationUpdated([&](const anychat::Conversation&) {
        ++call_count;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "friend.request",
            "timestamp": 1708329602,
            "data": { "fromUserId": "user-X" }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_EQ(call_count, 0);
}
