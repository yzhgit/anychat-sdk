#include <gtest/gtest.h>
#include "sync_engine.h"
#include "cache/conversation_cache.h"
#include "cache/message_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <string>

// ===========================================================================
// Fixture
// ===========================================================================
class SyncEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<anychat::db::Database>(":memory:");
        ASSERT_TRUE(db_->open());

        conv_cache_ = std::make_unique<anychat::cache::ConversationCache>();
        msg_cache_  = std::make_unique<anychat::cache::MessageCache>();

        http_ = std::make_shared<anychat::network::HttpClient>(
            "http://localhost:19999");

        engine_ = std::make_unique<anychat::SyncEngine>(
            db_.get(), conv_cache_.get(), msg_cache_.get(), http_);
    }

    void TearDown() override {
        engine_.reset();
        http_.reset();
        msg_cache_.reset();
        conv_cache_.reset();
        db_->close();
        db_.reset();
    }

    void drainDb() { db_->querySync("SELECT 1"); }

    std::unique_ptr<anychat::db::Database>              db_;
    std::unique_ptr<anychat::cache::ConversationCache>  conv_cache_;
    std::unique_ptr<anychat::cache::MessageCache>       msg_cache_;
    std::shared_ptr<anychat::network::HttpClient>       http_;
    std::unique_ptr<anychat::SyncEngine>                engine_;
};

// ---------------------------------------------------------------------------
// 1. SyncDoesNotCrashWithNoNetwork
//    sync() fires an async HTTP request; since the server is unreachable the
//    request just fails silently.  No crash or exception should propagate.
// ---------------------------------------------------------------------------
TEST_F(SyncEngineTest, SyncDoesNotCrashWithNoNetwork) {
    EXPECT_NO_THROW(engine_->sync());
}

// ---------------------------------------------------------------------------
// 2. SyncCalledMultipleTimes
//    Calling sync() multiple times (e.g. on every reconnect) should be safe.
// ---------------------------------------------------------------------------
TEST_F(SyncEngineTest, SyncCalledMultipleTimes) {
    EXPECT_NO_THROW({
        engine_->sync();
        engine_->sync();
        engine_->sync();
    });
}

// ---------------------------------------------------------------------------
// 3. ConversationCacheEmptyBeforeSync
//    Before any sync, the conversation cache should return an empty list.
// ---------------------------------------------------------------------------
TEST_F(SyncEngineTest, ConversationCacheEmptyBeforeSync) {
    auto convs = conv_cache_->getAll();
    EXPECT_TRUE(convs.empty());
}

// ---------------------------------------------------------------------------
// 4. MessageCacheEmptyBeforeSync
//    Before any sync, no messages should be in the cache for any conversation.
// ---------------------------------------------------------------------------
TEST_F(SyncEngineTest, MessageCacheEmptyBeforeSync) {
    auto msgs = msg_cache_->get("conv-any");
    EXPECT_TRUE(msgs.empty());
}
