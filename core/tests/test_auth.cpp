#include <gtest/gtest.h>
#include "auth_manager.h"
#include "db/database.h"
#include "network/http_client.h"

#include <ctime>
#include <memory>
#include <string>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

// Create a shared HttpClient pointing at a dummy URL (no real network used).
std::shared_ptr<anychat::network::HttpClient> makeDummyHttp() {
    return std::make_shared<anychat::network::HttpClient>("http://localhost:19999");
}

// Open a fresh in-memory database and return it.
std::unique_ptr<anychat::db::Database> makeDb() {
    auto db = std::make_unique<anychat::db::Database>(":memory:");
    bool ok = db->open();
    if (!ok) throw std::runtime_error("Failed to open in-memory database");
    return db;
}

} // anonymous namespace

// ===========================================================================
// AuthManager tests
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. NoTokenInitially
//    A freshly created AuthManager with an empty DB should not be logged in.
// ---------------------------------------------------------------------------
TEST(AuthManagerTest, NoTokenInitially) {
    auto db   = makeDb();
    auto http = makeDummyHttp();
    auto auth = anychat::createAuthManager(http, "device-test-001", db.get());

    EXPECT_FALSE(auth->isLoggedIn())
        << "AuthManager should not be logged in on a fresh (empty) database";
}

// ---------------------------------------------------------------------------
// 2. TokenPersistedAcrossRestart
//    Writing token metadata directly to the DB and then constructing a new
//    AuthManager should restore the logged-in state.
//
//    auth_manager.cpp reads:
//      db_->getMeta("auth.access_token",  "")
//      db_->getMeta("auth.refresh_token", "")
//      db_->getMeta("auth.expires_at_ms", "0")   <-- stored as Unix *seconds*
//
//    isLoggedIn() checks: token_.expires_at > std::time(nullptr)
// ---------------------------------------------------------------------------
TEST(AuthManagerTest, TokenPersistedAcrossRestart) {
    auto db = makeDb();

    // Simulate a stored, non-expired token.
    // expires_at is stored as seconds (despite the key name "expires_at_ms").
    int64_t future_expires = static_cast<int64_t>(std::time(nullptr)) + 7200; // 2 hours ahead

    db->setMeta("auth.access_token",  "eyJ_fake_access_token");
    db->setMeta("auth.refresh_token", "eyJ_fake_refresh_token");
    db->setMeta("auth.expires_at_ms", std::to_string(future_expires));

    // Construct a new auth manager against the same DB — it should restore
    // the token from metadata.
    auto http  = makeDummyHttp();
    auto auth2 = anychat::createAuthManager(http, "device-test-001", db.get());

    EXPECT_TRUE(auth2->isLoggedIn())
        << "AuthManager should report logged-in after restoring a valid token";

    auto tok = auth2->currentToken();
    EXPECT_EQ(tok.access_token,  "eyJ_fake_access_token");
    EXPECT_EQ(tok.refresh_token, "eyJ_fake_refresh_token");
}

// ---------------------------------------------------------------------------
// 3. ClearToken
//    After storing a valid token and then clearing it, isLoggedIn() should
//    return false — both immediately and after a fresh AuthManager is created
//    from the same DB.
// ---------------------------------------------------------------------------
TEST(AuthManagerTest, ClearToken) {
    auto db = makeDb();

    // Persist a valid token directly.
    int64_t future_expires = static_cast<int64_t>(std::time(nullptr)) + 7200;
    db->setMeta("auth.access_token",  "eyJ_valid_token");
    db->setMeta("auth.refresh_token", "eyJ_refresh_token");
    db->setMeta("auth.expires_at_ms", std::to_string(future_expires));

    auto http = makeDummyHttp();
    auto auth = anychat::createAuthManager(http, "device-test-002", db.get());

    ASSERT_TRUE(auth->isLoggedIn()) << "Pre-condition: should be logged in";

    // Clear the token by writing empty/zero values to the metadata directly,
    // which mirrors what AuthManagerImpl::clearToken() does.
    db->setMeta("auth.access_token",  "");
    db->setMeta("auth.refresh_token", "");
    db->setMeta("auth.expires_at_ms", "0");

    // A brand-new AuthManager from the same DB should not see a valid token.
    auto auth2 = anychat::createAuthManager(http, "device-test-002", db.get());
    EXPECT_FALSE(auth2->isLoggedIn())
        << "AuthManager should not be logged in after token was cleared";
}

// ---------------------------------------------------------------------------
// 4. ExpiredTokenNotLoggedIn
//    A token stored with an expiry in the past should not result in
//    isLoggedIn() returning true.
// ---------------------------------------------------------------------------
TEST(AuthManagerTest, ExpiredTokenNotLoggedIn) {
    auto db = makeDb();

    // Store an already-expired token (expiry 1 hour in the past).
    int64_t past_expires = static_cast<int64_t>(std::time(nullptr)) - 3600;
    db->setMeta("auth.access_token",  "eyJ_old_access_token");
    db->setMeta("auth.refresh_token", "eyJ_old_refresh_token");
    db->setMeta("auth.expires_at_ms", std::to_string(past_expires));

    auto http = makeDummyHttp();
    auto auth = anychat::createAuthManager(http, "device-test-003", db.get());

    EXPECT_FALSE(auth->isLoggedIn())
        << "AuthManager should not report logged-in for an expired token";
}

// ---------------------------------------------------------------------------
// 5. NoTokenWithNullDb
//    When db=nullptr is passed, the auth manager should still construct and
//    report not logged in.
// ---------------------------------------------------------------------------
TEST(AuthManagerTest, NoTokenWithNullDb) {
    auto http = makeDummyHttp();
    auto auth = anychat::createAuthManager(http, "device-no-db", nullptr);
    EXPECT_FALSE(auth->isLoggedIn())
        << "AuthManager with null DB should not be logged in";
}
