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
    int error_code = 0;
    std::string error;

    mgr_->checkVersion(
        0,
        "1.2.3",
        0,
        anychat::AnyChatValueCallback<anychat::VersionCheckResult>{
            .on_error =
                [&](int code, const std::string& err) {
                    called = true;
                    error_code = code;
                    error = err;
                },
        }
    );

    EXPECT_TRUE(called);
    EXPECT_EQ(error_code, -1);
    EXPECT_FALSE(error.empty());

    called = false;
    error_code = 0;
    error.clear();
    mgr_->checkVersion(
        2,
        "",
        0,
        anychat::AnyChatValueCallback<anychat::VersionCheckResult>{
            .on_error =
                [&](int code, const std::string& err) {
                    called = true;
                    error_code = code;
                    error = err;
                },
        }
    );

    EXPECT_TRUE(called);
    EXPECT_EQ(error_code, -1);
    EXPECT_FALSE(error.empty());
}

TEST_F(VersionManagerTest, GetLatestVersionRequiresPlatform) {
    bool called = false;
    int error_code = 0;
    std::string error;

    mgr_->getLatestVersion(
        0,
        1,
        anychat::AnyChatValueCallback<anychat::AppVersionInfo>{
            .on_error =
                [&](int code, const std::string& err) {
                    called = true;
                    error_code = code;
                    error = err;
                },
        }
    );

    EXPECT_TRUE(called);
    EXPECT_EQ(error_code, -1);
    EXPECT_FALSE(error.empty());
}

TEST_F(VersionManagerTest, ListVersionsDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->listVersions(2, 1, 1, 20, {}));
}

TEST_F(VersionManagerTest, ReportVersionRequiresPlatformAndVersion) {
    bool called = false;
    int error_code = 0;
    std::string error;

    mgr_->reportVersion(
        0,
        "1.0.0",
        100,
        "device-1",
        "Android 14",
        "0.1.0",
        anychat::AnyChatCallback{
            .on_error =
                [&](int code, const std::string& err) {
                    called = true;
                    error_code = code;
                    error = err;
                },
        }
    );

    EXPECT_TRUE(called);
    EXPECT_EQ(error_code, -1);
    EXPECT_FALSE(error.empty());
}
