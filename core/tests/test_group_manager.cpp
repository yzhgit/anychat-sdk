#include <gtest/gtest.h>
#include "group_manager.h"
#include "notification_manager.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <string>

// ===========================================================================
// Fixture
// ===========================================================================
class GroupManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_        = std::make_unique<anychat::db::Database>(":memory:");
        ASSERT_TRUE(db_->open());
        notif_mgr_ = std::make_unique<anychat::NotificationManager>();
        http_      = std::make_shared<anychat::network::HttpClient>(
            "http://localhost:19999");
        mgr_ = std::make_unique<anychat::GroupManagerImpl>(
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
    std::unique_ptr<anychat::GroupManagerImpl>      mgr_;
};

// ---------------------------------------------------------------------------
// 1. GroupInvitedNotificationFiresHandler
//    A group.invited notification should invoke the OnGroupInvited handler.
// ---------------------------------------------------------------------------
TEST_F(GroupManagerTest, GroupInvitedNotificationFiresHandler) {
    anychat::Group received{};
    int call_count = 0;

    mgr_->setOnGroupInvited([&](const anychat::Group& g, const std::string& /*inviter_id*/) {
        received = g;
        ++call_count;
    });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "group.invited",
            "timestamp": 1708329600,
            "data": {
                "groupId": "grp-001",
                "groupName": "项目讨论组",
                "inviterId": "user-inviter-111"
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    ASSERT_EQ(call_count, 1);
    EXPECT_EQ(received.group_id, "grp-001");
}

// ---------------------------------------------------------------------------
// 2. GroupInfoUpdatedNotificationFiresUpdatedHandler
//    A group.info_updated notification should invoke OnGroupUpdated.
// ---------------------------------------------------------------------------
TEST_F(GroupManagerTest, GroupInfoUpdatedNotificationFiresUpdatedHandler) {
    int call_count = 0;
    mgr_->setOnGroupUpdated([&](const anychat::Group&) { ++call_count; });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "group.info_updated",
            "timestamp": 1708329601,
            "data": {
                "groupId": "grp-002",
                "groupName": "新名称"
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_GE(call_count, 1);
}

// ---------------------------------------------------------------------------
// 3. UnrelatedNotificationDoesNotFireHandlers
//    Friend notifications must not trigger group handlers.
// ---------------------------------------------------------------------------
TEST_F(GroupManagerTest, UnrelatedNotificationDoesNotFireHandlers) {
    int invited_count = 0;
    int updated_count = 0;
    mgr_->setOnGroupInvited([&](const anychat::Group&, const std::string&) { ++invited_count; });
    mgr_->setOnGroupUpdated([&](const anychat::Group&) { ++updated_count; });

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notificationType": "friend.request",
            "timestamp": 1708329602,
            "data": { "fromUserId": "user-X" }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_EQ(invited_count, 0);
    EXPECT_EQ(updated_count, 0);
}

// ---------------------------------------------------------------------------
// 4. GetListDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(GroupManagerTest, GetListDoesNotCrash) {
    EXPECT_NO_THROW(
        mgr_->getList([](const std::vector<anychat::Group>&, const std::string&) {})
    );
}
