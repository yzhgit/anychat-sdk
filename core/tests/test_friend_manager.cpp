#include <gtest/gtest.h>
#include "friend_manager.h"
#include "notification_manager.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <string>

// ===========================================================================
// Fixture
// ===========================================================================
class FriendManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_        = std::make_unique<anychat::db::Database>(":memory:");
        ASSERT_TRUE(db_->open());
        notif_mgr_ = std::make_unique<anychat::NotificationManager>();
        http_      = std::make_shared<anychat::network::HttpClient>(
            "http://localhost:19999");
        mgr_ = std::make_unique<anychat::FriendManagerImpl>(
            db_.get(), notif_mgr_.get(), http_);
    }

    void TearDown() override {
        mgr_.reset();
        http_.reset();
        notif_mgr_.reset();
        db_->close();
        db_.reset();
    }

    std::unique_ptr<anychat::db::Database>          db_;
    std::unique_ptr<anychat::NotificationManager>   notif_mgr_;
    std::shared_ptr<anychat::network::HttpClient>   http_;
    std::unique_ptr<anychat::FriendManagerImpl>     mgr_;
};

// ---------------------------------------------------------------------------
// 1. FriendRequestNotificationFiresHandler
//    A friend.request WebSocket notification should invoke OnFriendRequest.
// ---------------------------------------------------------------------------
TEST_F(FriendManagerTest, FriendRequestNotificationFiresHandler) {
    anychat::FriendRequest received{};
    int call_count = 0;

    mgr_->setOnFriendRequest([&](const anychat::FriendRequest& req) {
        received = req;
        ++call_count;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "friend.request",
            "timestamp": 1708329600,
            "data": {
                "requestId": "req-001",
                "fromUserId": "user-sender-111",
                "message": "你好，加个好友"
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    ASSERT_EQ(call_count, 1);
    EXPECT_EQ(received.from_user_id, "user-sender-111");
}

// ---------------------------------------------------------------------------
// 2. FriendDeletedNotificationFiresListChangedHandler
//    A friend.deleted notification should invoke OnFriendListChanged.
// ---------------------------------------------------------------------------
TEST_F(FriendManagerTest, FriendDeletedNotificationFiresListChangedHandler) {
    int call_count = 0;
    mgr_->setOnFriendListChanged([&]() { ++call_count; });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "friend.deleted",
            "timestamp": 1708329601,
            "data": { "userId": "user-deleted-222" }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_GE(call_count, 1);
}

// ---------------------------------------------------------------------------
// 3. UnrelatedNotificationDoesNotFireHandlers
//    Group notifications must not trigger friend handlers.
// ---------------------------------------------------------------------------
TEST_F(FriendManagerTest, UnrelatedNotificationDoesNotFireHandlers) {
    int req_count  = 0;
    int list_count = 0;
    mgr_->setOnFriendRequest([&](const anychat::FriendRequest&) { ++req_count; });
    mgr_->setOnFriendListChanged([&]() { ++list_count; });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "group.invited",
            "timestamp": 1708329602,
            "data": { "groupId": "grp-999" }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_EQ(req_count,  0);
    EXPECT_EQ(list_count, 0);
}

// ---------------------------------------------------------------------------
// 4. GetListDoesNotCrash
//    getList() initiates an HTTP call and invokes the callback (possibly with
//    an error since no server is running).  No crash is acceptable.
// ---------------------------------------------------------------------------
TEST_F(FriendManagerTest, GetListDoesNotCrash) {
    EXPECT_NO_THROW(
        mgr_->getList([](const std::vector<anychat::Friend>&, const std::string&) {})
    );
}
