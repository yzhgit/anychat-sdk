#include <gtest/gtest.h>
#include "cache/lru_cache.h"
#include "cache/conversation_cache.h"
#include "cache/message_cache.h"
#include "anychat/types.h"

// ===========================================================================
// LruCache tests
// ===========================================================================

TEST(LruCacheTest, BasicGetPut) {
    anychat::cache::LruCache<std::string, int> cache(10);

    cache.put("a", 1);
    auto result = cache.get("a");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 1);

    auto missing = cache.get("b");
    EXPECT_FALSE(missing.has_value());
}

TEST(LruCacheTest, Eviction) {
    // Capacity 2: inserting a third entry evicts the LRU entry.
    anychat::cache::LruCache<std::string, int> cache(2);

    cache.put("a", 1);
    cache.put("b", 2);
    // "a" is now LRU. Insert "c" — "a" should be evicted.
    cache.put("c", 3);

    EXPECT_FALSE(cache.get("a").has_value()) << "LRU entry 'a' should have been evicted";
    EXPECT_TRUE(cache.get("b").has_value());
    EXPECT_TRUE(cache.get("c").has_value());
}

TEST(LruCacheTest, LruOrdering) {
    // Capacity 2: put a, put b, get a (promotes a), put c → b evicted.
    anychat::cache::LruCache<std::string, int> cache(2);

    cache.put("a", 1);
    cache.put("b", 2);
    // Access "a" to make it most-recently-used. "b" becomes LRU.
    auto val_a = cache.get("a");
    ASSERT_TRUE(val_a.has_value());

    // Insert "c": "b" is now LRU and should be evicted.
    cache.put("c", 3);

    EXPECT_TRUE(cache.get("a").has_value())  << "'a' should still be present";
    EXPECT_FALSE(cache.get("b").has_value()) << "'b' should have been evicted";
    EXPECT_TRUE(cache.get("c").has_value())  << "'c' should be present";
}

TEST(LruCacheTest, UpdateExistingKey) {
    anychat::cache::LruCache<std::string, int> cache(5);
    cache.put("x", 10);
    cache.put("x", 20); // update
    auto result = cache.get("x");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 20);
    EXPECT_EQ(cache.size(), 1u);
}

TEST(LruCacheTest, Remove) {
    anychat::cache::LruCache<std::string, int> cache(5);
    cache.put("r", 99);
    cache.remove("r");
    EXPECT_FALSE(cache.get("r").has_value());
    EXPECT_EQ(cache.size(), 0u);
}

TEST(LruCacheTest, Clear) {
    anychat::cache::LruCache<std::string, int> cache(5);
    cache.put("p", 1);
    cache.put("q", 2);
    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_FALSE(cache.contains("p"));
}

// ===========================================================================
// MessageCache tests
// ===========================================================================

namespace {
// Helper to build a minimal Message for the cache tests.
anychat::Message makeMsg(const std::string& conv_id,
                          const std::string& message_id,
                          int64_t seq) {
    anychat::Message m;
    m.conv_id    = conv_id;
    m.message_id = message_id;
    m.seq        = seq;
    return m;
}
} // anonymous namespace

TEST(MessageCacheTest, InsertAndGet) {
    anychat::cache::MessageCache cache;
    cache.insert(makeMsg("c1", "msg-1", 1));

    auto msgs = cache.get("c1");
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].message_id, "msg-1");
    EXPECT_EQ(msgs[0].seq, 1);
}

TEST(MessageCacheTest, Dedup) {
    // Inserting the same message_id twice should result in only one entry.
    anychat::cache::MessageCache cache;
    cache.insert(makeMsg("c1", "msg-dup", 1));
    cache.insert(makeMsg("c1", "msg-dup", 1));

    auto msgs = cache.get("c1");
    EXPECT_EQ(msgs.size(), 1u);
}

TEST(MessageCacheTest, GapDetection) {
    anychat::cache::MessageCache cache;
    cache.insert(makeMsg("c1", "msg-5", 5));

    // Next expected seq is 6. Incoming seq=7 means there is a gap.
    EXPECT_TRUE(cache.hasGapBefore("c1", 7))
        << "Gap expected: cached max_seq=5, incoming seq=7";

    // seq=6 is the next expected — no gap.
    EXPECT_FALSE(cache.hasGapBefore("c1", 6))
        << "No gap: cached max_seq=5, incoming seq=6 (next in sequence)";
}

TEST(MessageCacheTest, GapDetectionEmptyCache) {
    anychat::cache::MessageCache cache;
    // Empty cache: seq=1 means it's the first message, no gap.
    EXPECT_FALSE(cache.hasGapBefore("new_conv", 1));
    // Empty cache: seq=5 means we are missing 1-4, so gap.
    EXPECT_TRUE(cache.hasGapBefore("new_conv", 5));
}

TEST(MessageCacheTest, BucketEviction) {
    // bucket_size=3: inserting 4 messages should keep the 3 most recent by seq.
    anychat::cache::MessageCache cache(/*bucket_size=*/3);

    cache.insert(makeMsg("c1", "msg-1", 1));
    cache.insert(makeMsg("c1", "msg-2", 2));
    cache.insert(makeMsg("c1", "msg-3", 3));
    cache.insert(makeMsg("c1", "msg-4", 4)); // should evict msg-1 (lowest seq)

    auto msgs = cache.get("c1");
    ASSERT_EQ(msgs.size(), 3u) << "Bucket should hold exactly 3 messages";

    // Verify seq 1 was evicted and seq 2,3,4 remain.
    EXPECT_EQ(msgs[0].seq, 2);
    EXPECT_EQ(msgs[1].seq, 3);
    EXPECT_EQ(msgs[2].seq, 4);
}

TEST(MessageCacheTest, MaxSeq) {
    anychat::cache::MessageCache cache;
    EXPECT_EQ(cache.maxSeq("nonexistent"), 0);

    cache.insert(makeMsg("c1", "m1", 3));
    cache.insert(makeMsg("c1", "m2", 7));
    cache.insert(makeMsg("c1", "m3", 2));
    EXPECT_EQ(cache.maxSeq("c1"), 7);
}

TEST(MessageCacheTest, RemoveConversation) {
    anychat::cache::MessageCache cache;
    cache.insert(makeMsg("c1", "m1", 1));
    cache.removeConversation("c1");
    EXPECT_TRUE(cache.get("c1").empty());
}

// ===========================================================================
// ConversationCache tests
// ===========================================================================

namespace {
// Helper to build a minimal Conversation for the cache tests.
anychat::Conversation makeConv(const std::string& conv_id,
                                bool is_pinned,
                                int64_t last_msg_time_ms,
                                int64_t pin_time_ms = 0) {
    anychat::Conversation c;
    c.conv_id          = conv_id;
    c.is_pinned        = is_pinned;
    c.last_msg_time_ms = last_msg_time_ms;
    c.pin_time_ms      = pin_time_ms;
    return c;
}
} // anonymous namespace

TEST(ConversationCacheTest, UpsertAndGet) {
    anychat::cache::ConversationCache cache;

    anychat::Conversation conv = makeConv("conv-1", false, 1000);
    conv.target_id = "user-abc";
    cache.upsert(conv);

    auto result = cache.get("conv-1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->conv_id,   "conv-1");
    EXPECT_EQ(result->target_id, "user-abc");
}

TEST(ConversationCacheTest, UpsertUpdatesExisting) {
    anychat::cache::ConversationCache cache;
    cache.upsert(makeConv("conv-1", false, 1000));

    anychat::Conversation updated = makeConv("conv-1", false, 2000);
    updated.last_msg_text = "hello";
    cache.upsert(updated);

    auto result = cache.get("conv-1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->last_msg_time_ms, 2000);
    EXPECT_EQ(result->last_msg_text, "hello");
}

TEST(ConversationCacheTest, SortOrderPinnedFirst) {
    anychat::cache::ConversationCache cache;
    // Insert unpinned then pinned — the sorted list should put pinned first.
    cache.upsert(makeConv("unpinned", false, 5000));
    cache.upsert(makeConv("pinned",   true,  1000, /*pin_time=*/100));

    auto all = cache.getAll();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0].conv_id, "pinned")   << "Pinned conversation should come first";
    EXPECT_EQ(all[1].conv_id, "unpinned") << "Unpinned conversation should come second";
}

TEST(ConversationCacheTest, SortOrderUnpinnedByTime) {
    anychat::cache::ConversationCache cache;
    // Among unpinned, higher last_msg_time_ms should come first.
    cache.upsert(makeConv("older",  false, 1000));
    cache.upsert(makeConv("newer",  false, 9000));
    cache.upsert(makeConv("middle", false, 5000));

    auto all = cache.getAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].conv_id, "newer");
    EXPECT_EQ(all[1].conv_id, "middle");
    EXPECT_EQ(all[2].conv_id, "older");
}

TEST(ConversationCacheTest, RemoveConversation) {
    anychat::cache::ConversationCache cache;
    cache.upsert(makeConv("c1", false, 1000));
    cache.remove("c1");
    EXPECT_FALSE(cache.get("c1").has_value());
    EXPECT_TRUE(cache.getAll().empty());
}

TEST(ConversationCacheTest, IncrementAndClearUnread) {
    anychat::cache::ConversationCache cache;
    cache.upsert(makeConv("c1", false, 1000));

    cache.incrementUnread("c1");
    cache.incrementUnread("c1");
    auto result = cache.get("c1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->unread_count, 2);

    cache.clearUnread("c1");
    result = cache.get("c1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->unread_count, 0);
}

TEST(ConversationCacheTest, MutedConvDoesNotIncrementUnread) {
    anychat::cache::ConversationCache cache;
    anychat::Conversation muted = makeConv("muted-conv", false, 1000);
    muted.is_muted = true;
    cache.upsert(muted);

    cache.incrementUnread("muted-conv");
    auto result = cache.get("muted-conv");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->unread_count, 0) << "Muted conversation should not accumulate unread";
}

TEST(ConversationCacheTest, SetLastMessage) {
    anychat::cache::ConversationCache cache;
    cache.upsert(makeConv("c1", false, 1000));

    cache.setLastMessage("c1", "msg-99", "Hello!", 9999);
    auto result = cache.get("c1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->last_msg_id,      "msg-99");
    EXPECT_EQ(result->last_msg_text,    "Hello!");
    EXPECT_EQ(result->last_msg_time_ms, 9999);
}

TEST(ConversationCacheTest, SetAll) {
    anychat::cache::ConversationCache cache;
    cache.upsert(makeConv("old", false, 100)); // will be replaced

    std::vector<anychat::Conversation> fresh;
    fresh.push_back(makeConv("new1", false, 200));
    fresh.push_back(makeConv("new2", false, 300));
    cache.setAll(std::move(fresh));

    EXPECT_FALSE(cache.get("old").has_value());
    EXPECT_TRUE(cache.get("new1").has_value());
    EXPECT_TRUE(cache.get("new2").has_value());
}
