#include "group_manager.h"
#include "json_common.h"
#include "notification_manager.h"

#include "db/database.h"
#include "network/http_client.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

anychat::AnyChatCallback makeNoopCallback() {
    anychat::AnyChatCallback callback{};
    callback.on_success = []() {};
    callback.on_error = [](int, const std::string&) {};
    return callback;
}

template <typename T>
anychat::AnyChatValueCallback<T> makeNoopValueCallback() {
    anychat::AnyChatValueCallback<T> callback{};
    callback.on_success = [](const T&) {};
    callback.on_error = [](int, const std::string&) {};
    return callback;
}

} // namespace

class GroupManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<anychat::db::Database>(":memory:");
        ASSERT_TRUE(db_->open());
        notif_mgr_ = std::make_unique<anychat::NotificationManager>();
        http_ = std::make_shared<anychat::network::HttpClient>("http://localhost:19999");
        mgr_ = std::make_unique<anychat::GroupManagerImpl>(db_.get(), notif_mgr_.get(), http_);
    }

    void TearDown() override {
        mgr_.reset();
        http_.reset();
        notif_mgr_.reset();
        db_->close();
        db_.reset();
    }

    std::unique_ptr<anychat::db::Database> db_;
    std::unique_ptr<anychat::NotificationManager> notif_mgr_;
    std::shared_ptr<anychat::network::HttpClient> http_;
    std::unique_ptr<anychat::GroupManagerImpl> mgr_;
};

class TestGroupListener final : public anychat::GroupListener {
public:
    std::function<void(const anychat::Group&, const std::string&)> on_invited;
    std::function<void(const anychat::Group&)> on_updated;

    void onGroupInvited(const anychat::Group& group, const std::string& inviter_id) override {
        if (on_invited) {
            on_invited(group, inviter_id);
        }
    }

    void onGroupUpdated(const anychat::Group& group) override {
        if (on_updated) {
            on_updated(group);
        }
    }
};

struct GroupCreatePayloadForTest {
    std::string group_id{};
    std::string name{};
};

TEST_F(GroupManagerTest, GroupInvitedNotificationFiresHandler) {
    anychat::Group received{};
    std::string inviter_id;
    int call_count = 0;

    auto listener = std::make_shared<TestGroupListener>();
    listener->on_invited = [&](const anychat::Group& group, const std::string& inviter) {
        received = group;
        inviter_id = inviter;
        ++call_count;
    };
    mgr_->setListener(listener);

    notif_mgr_->handleRaw(R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-group-invite-001",
            "type": "group.invited",
            "timestamp": 1708329600,
            "payload": {
                "group_id": "grp-001",
                "group_name": "demo-group",
                "inviter_user_id": "user-inviter-111"
            }
        }
    })");

    ASSERT_EQ(call_count, 1);
    EXPECT_EQ(received.group_id, "grp-001");
    EXPECT_EQ(received.name, "demo-group");
    EXPECT_EQ(inviter_id, "user-inviter-111");
}

TEST_F(GroupManagerTest, GroupInfoUpdatedNotificationFiresUpdatedHandler) {
    anychat::Group updated{};
    int call_count = 0;
    auto listener = std::make_shared<TestGroupListener>();
    listener->on_updated = [&](const anychat::Group& group) {
        updated = group;
        ++call_count;
    };
    mgr_->setListener(listener);

    notif_mgr_->handleRaw(R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-group-update-001",
            "type": "group.info_updated",
            "timestamp": 1708329601,
            "payload": {
                "group_id": "grp-002",
                "group_name": "updated-group",
                "group_avatar": "https://cdn.example/avatar.png",
                "updated_fields": ["name", "avatar"]
            }
        }
    })");

    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(updated.group_id, "grp-002");
    EXPECT_EQ(updated.name, "updated-group");
    EXPECT_EQ(updated.avatar_url, "https://cdn.example/avatar.png");
}

TEST_F(GroupManagerTest, UnrelatedNotificationDoesNotFireHandlers) {
    int invited_count = 0;
    int updated_count = 0;
    auto listener = std::make_shared<TestGroupListener>();
    listener->on_invited = [&](const anychat::Group&, const std::string&) {
        ++invited_count;
    };
    listener->on_updated = [&](const anychat::Group&) {
        ++updated_count;
    };
    mgr_->setListener(listener);

    notif_mgr_->handleRaw(R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-friend-003",
            "type": "friend.request",
            "timestamp": 1708329602,
            "payload": { "from_user_id": "user-X" }
        }
    })");

    EXPECT_EQ(invited_count, 0);
    EXPECT_EQ(updated_count, 0);
}

TEST_F(GroupManagerTest, GroupMutedNotificationFiresUpdatedHandler) {
    anychat::Group updated{};
    int call_count = 0;

    auto listener = std::make_shared<TestGroupListener>();
    listener->on_updated = [&](const anychat::Group& group) {
        updated = group;
        ++call_count;
    };
    mgr_->setListener(listener);

    notif_mgr_->handleRaw(R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-group-muted-001",
            "type": "group.muted",
            "timestamp": 1708329603,
            "payload": {
                "group_id": "grp-003",
                "is_muted": true
            }
        }
    })");

    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(updated.group_id, "grp-003");
    EXPECT_TRUE(updated.is_muted);
}

TEST_F(GroupManagerTest, GetGroupListDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getGroupList(makeNoopValueCallback<std::vector<anychat::Group>>()));
}

TEST_F(GroupManagerTest, GetInfoDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getInfo("grp-001", makeNoopValueCallback<anychat::Group>()));
}

TEST_F(GroupManagerTest, CreateDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->create("demo-group", {"user-001", "user-002"}, makeNoopValueCallback<anychat::Group>()));
}

TEST_F(GroupManagerTest, JoinDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->join("grp-001", "hello", makeNoopCallback()));
}

TEST_F(GroupManagerTest, InviteDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->invite("grp-001", {"user-003", "user-004"}, makeNoopCallback()));
}

TEST_F(GroupManagerTest, QuitDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->quit("grp-001", makeNoopCallback()));
}

TEST_F(GroupManagerTest, DisbandDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->disband("grp-001", makeNoopCallback()));
}

TEST_F(GroupManagerTest, UpdateDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->update("grp-001", "new-name", "https://cdn.example/new.png", makeNoopCallback()));
}

TEST_F(GroupManagerTest, GetMembersDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getMembers("grp-001", 1, 20, makeNoopValueCallback<std::vector<anychat::GroupMember>>()));
}

TEST_F(GroupManagerTest, RemoveMemberDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->removeMember("grp-001", "user-002", makeNoopCallback()));
}

TEST_F(GroupManagerTest, UpdateMemberRoleDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->updateMemberRole("grp-001", "user-002", 2, makeNoopCallback()));
}

TEST_F(GroupManagerTest, UpdateNicknameDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->updateNickname("grp-001", "tester", makeNoopCallback()));
}

TEST_F(GroupManagerTest, TransferOwnershipDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->transferOwnership("grp-001", "user-009", makeNoopCallback()));
}

TEST_F(GroupManagerTest, GetJoinRequestsDoesNotCrash) {
    EXPECT_NO_THROW(
        mgr_->getJoinRequests("grp-001", 1, makeNoopValueCallback<std::vector<anychat::GroupJoinRequest>>())
    );
}

TEST_F(GroupManagerTest, HandleJoinRequestDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->handleJoinRequest("grp-001", 1001, true, makeNoopCallback()));
}

TEST_F(GroupManagerTest, GetQRCodeDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getQRCode("grp-001", makeNoopValueCallback<anychat::GroupQRCode>()));
}

TEST_F(GroupManagerTest, RefreshQRCodeDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->refreshQRCode("grp-001", makeNoopValueCallback<anychat::GroupQRCode>()));
}

TEST(GroupJsonCommonTest, ParseApiEnvelopeResponsePreservesServerCodeAndMessage) {
    anychat::network::HttpResponse resp{};
    resp.status_code = 403;
    resp.body = R"({
        "code": 40106,
        "message": "already a group member",
        "data": {
            "group_id": "grp-001",
            "name": "demo-group"
        }
    })";

    anychat::json_common::ApiEnvelope<GroupCreatePayloadForTest> root{};

    EXPECT_FALSE(anychat::json_common::parseApiEnvelopeResponse(resp, root, "create group failed"));
    EXPECT_EQ(root.code, 40106);
    EXPECT_EQ(root.message, "already a group member");
}

TEST(GroupJsonCommonTest, ParseApiEnvelopeResponseReturnsPayloadOnSuccess) {
    anychat::network::HttpResponse resp{};
    resp.status_code = 200;
    resp.body = R"({
        "code": 0,
        "message": "success",
        "data": {
            "group_id": "grp-002",
            "name": "created-group"
        }
    })";

    anychat::json_common::ApiEnvelope<GroupCreatePayloadForTest> root{};

    ASSERT_TRUE(anychat::json_common::parseApiEnvelopeResponse(resp, root, "create group failed"));
    EXPECT_EQ(root.code, 0);
    EXPECT_EQ(root.data.group_id, "grp-002");
    EXPECT_EQ(root.data.name, "created-group");
}
