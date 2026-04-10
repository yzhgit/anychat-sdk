#include "version_manager.h"

#include "network/http_client.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

class VersionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        http_ = std::make_shared<anychat::network::HttpClient>("http://localhost:19999");
        mgr_ = std::make_unique<anychat::VersionManagerImpl>(http_);
    }

    void TearDown() override {
        mgr_.reset();
        http_.reset();
    }

    std::shared_ptr<anychat::network::HttpClient> http_;
    std::unique_ptr<anychat::VersionManagerImpl> mgr_;
};

TEST_F(VersionManagerTest, CheckVersionRequiresPlatformAndVersion) {
    bool called = false;
    bool ok = true;
    std::string error;

    mgr_->checkVersion("", "1.2.3", 0, [&](bool success, const anychat::VersionCheckResult&, const std::string& err) {
        called = true;
        ok = success;
        error = err;
    });

    EXPECT_TRUE(called);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());

    called = false;
    ok = true;
    error.clear();
    mgr_->checkVersion("android", "", 0, [&](bool success, const anychat::VersionCheckResult&, const std::string& err) {
        called = true;
        ok = success;
        error = err;
    });

    EXPECT_TRUE(called);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST_F(VersionManagerTest, GetLatestVersionRequiresPlatform) {
    bool called = false;
    bool ok = true;
    std::string error;

    mgr_->getLatestVersion("", "stable", [&](bool success, const anychat::AppVersionInfo&, const std::string& err) {
        called = true;
        ok = success;
        error = err;
    });

    EXPECT_TRUE(called);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}

TEST_F(VersionManagerTest, ListVersionsDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->listVersions(
        "android",
        "stable",
        1,
        20,
        [](const std::vector<anychat::AppVersionInfo>&, int64_t, const std::string&) {}
    ));
}

TEST_F(VersionManagerTest, ReportVersionRequiresPlatformAndVersion) {
    bool called = false;
    bool ok = true;
    std::string error;

    mgr_->reportVersion(
        "",
        "1.0.0",
        100,
        "device-1",
        "Android 14",
        "0.1.0",
        [&](bool success, const std::string& err) {
            called = true;
            ok = success;
            error = err;
        }
    );

    EXPECT_TRUE(called);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(error.empty());
}
