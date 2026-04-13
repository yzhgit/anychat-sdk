#include "conversation_manager.h"
#include "notification_manager.h"

#include "cache/conversation_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <string>
#include <functional>

#include <gtest/gtest.h>

namespace {

anychat::AnyChatCallback makeNoopCallback() {
    anychat::AnyChatCallback callback{};
    callback.on_success = []() {};
    callback.on_error = [](int, const std::string&) {};
    return callback;
}

template<typename T>
anychat::AnyChatValueCallback<T> makeNoopValueCallback() {
    anychat::AnyChatValueCallback<T> callback{};
    callback.on_success = [](const T&) {};
    callback.on_error = [](int, const std::string&) {};
    return callback;
}

} // namespace

class ConversationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<anychat::db::Database>(":memory:");
        ASSERT_TRUE(db_->open());

        conv_cache_ = std::make_unique<anychat::cache::ConversationCache>();
        notif_mgr_ = std::make_unique<anychat::NotificationManager>();
        http_ = std::make_shared<anychat::network::HttpClient>("http://localhost:19999");
        mgr_ = std::make_unique<anychat::ConversationManagerImpl>(db_.get(), conv_cache_.get(), notif_mgr_.get(), http_);
    }

    void TearDown() override {
        mgr_.reset();
        http_.reset();
        notif_mgr_.reset();
        conv_cache_.reset();
        db_->close();
        db_.reset();
    }

    std::unique_ptr<anychat::db::Database> db_;
    std::unique_ptr<anychat::cache::ConversationCache> conv_cache_;
    std::unique_ptr<anychat::NotificationManager> notif_mgr_;
    std::shared_ptr<anychat::network::HttpClient> http_;
    std::unique_ptr<anychat::ConversationManagerImpl> mgr_;
};

class TestConversationListener final : public anychat::ConversationListener {
public:
    std::function<void(const anychat::Conversation&)> on_updated;

    void onConversationUpdated(const anychat::Conversation& conv) override {
        if (on_updated) {
            on_updated(conv);
        }
    }
};

TEST_F(ConversationManagerTest, GetListEmptyInitially) {
    EXPECT_TRUE(conv_cache_->getAll().empty());
}

TEST_F(ConversationManagerTest, GetConversationListDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getConversationList(makeNoopValueCallback<std::vector<anychat::Conversation>>()));
}

TEST_F(ConversationManagerTest, GetConversationDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getConversation("conv-001", makeNoopValueCallback<anychat::Conversation>()));
}

TEST_F(ConversationManagerTest, DeleteConversationDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->deleteConversation("conv-001", makeNoopCallback()));
}

TEST_F(ConversationManagerTest, MarkAllReadDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->markAllRead("conv-001", makeNoopCallback()));
}

TEST_F(ConversationManagerTest, MarkMessagesReadDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->markMessagesRead(
        "conv-001",
        std::vector<std::string>{ "msg-001", "msg-002" },
        makeNoopValueCallback<anychat::ConversationMarkReadResult>()
    ));
}

TEST_F(ConversationManagerTest, SetPinnedDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->setPinned("conv-001", true, makeNoopCallback()));
}

TEST_F(ConversationManagerTest, SetMutedDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->setMuted("conv-001", true, makeNoopCallback()));
}

TEST_F(ConversationManagerTest, SetBurnAfterReadingDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->setBurnAfterReading("conv-001", 60, makeNoopCallback()));
}

TEST_F(ConversationManagerTest, SetAutoDeleteDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->setAutoDelete("conv-001", 3600, makeNoopCallback()));
}

TEST_F(ConversationManagerTest, GetTotalUnreadDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getTotalUnread(makeNoopValueCallback<int32_t>()));
}

TEST_F(ConversationManagerTest, GetMessageUnreadCountDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getMessageUnreadCount("conv-001", 10, makeNoopValueCallback<anychat::ConversationUnreadState>()));
}

TEST_F(ConversationManagerTest, GetMessageReadReceiptsDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getMessageReadReceipts(
        "conv-001",
        makeNoopValueCallback<std::vector<anychat::ConversationReadReceipt>>()
    ));
}

TEST_F(ConversationManagerTest, GetMessageSequenceDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getMessageSequence("conv-001", makeNoopValueCallback<int64_t>()));
}

TEST_F(ConversationManagerTest, ConversationUnreadUpdatedNotificationFiresHandler) {
    anychat::Conversation updated{};
    int call_count = 0;
    auto listener = std::make_shared<TestConversationListener>();
    listener->on_updated = [&](const anychat::Conversation& conv) {
        updated = conv;
        ++call_count;
    };
    mgr_->setListener(listener);

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-conv-001",
            "type": "conversation.unread_updated",
            "timestamp": 1708329600,
            "payload": {
                "conversation_id": "conv-001",
                "unread_count": 3,
                "total_unread": 8
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_EQ(call_count, 1) << "OnConversationUpdated should fire on conversation.unread_updated";
    EXPECT_EQ(updated.conv_id, "conv-001");
    EXPECT_EQ(updated.unread_count, 3);
}

TEST_F(ConversationManagerTest, ConversationDeletedNotificationFiresHandler) {
    anychat::Conversation existing{};
    existing.conv_id = "conv-002";
    existing.last_msg_text = "cached";
    conv_cache_->upsert(existing);

    anychat::Conversation removed{};
    int call_count = 0;
    auto listener = std::make_shared<TestConversationListener>();
    listener->on_updated = [&](const anychat::Conversation& conv) {
        removed = conv;
        ++call_count;
    };
    mgr_->setListener(listener);

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-conv-002",
            "type": "conversation.deleted",
            "timestamp": 1708329601,
            "payload": {
                "conversation_id": "conv-002"
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(removed.conv_id, "conv-002");
    EXPECT_FALSE(conv_cache_->get("conv-002").has_value());
}

TEST_F(ConversationManagerTest, ConversationBurnUpdatedNotificationFiresHandler) {
    anychat::Conversation existing{};
    existing.conv_id = "conv-003";
    conv_cache_->upsert(existing);

    anychat::Conversation updated{};
    int call_count = 0;
    auto listener = std::make_shared<TestConversationListener>();
    listener->on_updated = [&](const anychat::Conversation& conv) {
        updated = conv;
        ++call_count;
    };
    mgr_->setListener(listener);

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-conv-003",
            "type": "conversation.burn_updated",
            "timestamp": 1708329602,
            "payload": {
                "conversation_id": "conv-003",
                "burn_after_reading": 60
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    ASSERT_EQ(call_count, 1);
    EXPECT_EQ(updated.conv_id, "conv-003");
    EXPECT_EQ(updated.burn_after_reading, 60);
}

TEST_F(ConversationManagerTest, ConversationAutoDeleteUpdatedNotificationFiresHandler) {
    anychat::Conversation existing{};
    existing.conv_id = "conv-004";
    conv_cache_->upsert(existing);

    anychat::Conversation updated{};
    int call_count = 0;
    auto listener = std::make_shared<TestConversationListener>();
    listener->on_updated = [&](const anychat::Conversation& conv) {
        updated = conv;
        ++call_count;
    };
    mgr_->setListener(listener);

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-conv-004",
            "type": "conversation.auto_delete_updated",
            "timestamp": 1708329603,
            "payload": {
                "conversation_id": "conv-004",
                "auto_delete_duration": 86400
            }
        }
    })";
    notif_mgr_->handleRaw(frame);

    ASSERT_EQ(call_count, 1);
    EXPECT_EQ(updated.conv_id, "conv-004");
    EXPECT_EQ(updated.auto_delete_duration, 86400);
}

TEST_F(ConversationManagerTest, UnrelatedNotificationDoesNotFireHandler) {
    int call_count = 0;
    auto listener = std::make_shared<TestConversationListener>();
    listener->on_updated = [&](const anychat::Conversation&) {
        ++call_count;
    };
    mgr_->setListener(listener);

    const std::string frame = R"({
        "type": "notification",
        "payload": {
            "notification_id": "notif-friend-002",
            "type": "friend.request",
            "timestamp": 1708329604,
            "payload": { "from_user_id": "user-X" }
        }
    })";
    notif_mgr_->handleRaw(frame);

    EXPECT_EQ(call_count, 0);
}
