#include <gtest/gtest.h>
#include "user_manager.h"
#include "network/http_client.h"

#include <memory>
#include <string>

// ===========================================================================
// Fixture
// ===========================================================================
class UserManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        http_ = std::make_shared<anychat::network::HttpClient>(
            "http://localhost:19999");
        mgr_ = std::make_unique<anychat::UserManagerImpl>(http_);
    }

    void TearDown() override {
        mgr_.reset();
        http_.reset();
    }

    std::shared_ptr<anychat::network::HttpClient>  http_;
    std::unique_ptr<anychat::UserManagerImpl>      mgr_;
};

// ---------------------------------------------------------------------------
// 1. GetProfileDoesNotCrash
//    getProfile() fires a GET /users/me; with no server the callback receives
//    an error, but no crash should occur.
// ---------------------------------------------------------------------------
TEST_F(UserManagerTest, GetProfileDoesNotCrash) {
    EXPECT_NO_THROW(
        mgr_->getProfile([](bool /*ok*/,
                            const anychat::UserProfile& /*p*/,
                            const std::string& /*err*/) {})
    );
}

// ---------------------------------------------------------------------------
// 2. UpdateProfileDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(UserManagerTest, UpdateProfileDoesNotCrash) {
    anychat::UserProfile p;
    p.nickname = "TestUser";
    EXPECT_NO_THROW(
        mgr_->updateProfile(p,
            [](bool /*ok*/,
               const anychat::UserProfile& /*p*/,
               const std::string& /*err*/) {})
    );
}

// ---------------------------------------------------------------------------
// 3. GetSettingsDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(UserManagerTest, GetSettingsDoesNotCrash) {
    EXPECT_NO_THROW(
        mgr_->getSettings([](bool /*ok*/,
                             const anychat::UserSettings& /*s*/,
                             const std::string& /*err*/) {})
    );
}

// ---------------------------------------------------------------------------
// 4. SearchUsersDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(UserManagerTest, SearchUsersDoesNotCrash) {
    EXPECT_NO_THROW(
        mgr_->searchUsers(
            "keyword",
            1, 20,
            [](const std::vector<anychat::UserInfo>& /*users*/,
               int64_t /*total*/,
               const std::string& /*err*/) {})
    );
}

// ---------------------------------------------------------------------------
// 5. UpdatePushTokenDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(UserManagerTest, UpdatePushTokenDoesNotCrash) {
    EXPECT_NO_THROW(
        mgr_->updatePushToken(
            "push-token-abc",
            "ios",
            [](bool /*ok*/, const std::string& /*err*/) {})
    );
}
