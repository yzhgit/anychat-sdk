#include "file_manager.h"

#include "anychat/types.h"
#include "network/http_client.h"

#include <memory>
#include <string>

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

// ===========================================================================
// Fixture
// ===========================================================================
class FileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        http_ = std::make_shared<anychat::network::HttpClient>("http://localhost:19999");
        mgr_ = std::make_unique<anychat::FileManagerImpl>(http_);
    }

    void TearDown() override {
        mgr_.reset();
        http_.reset();
    }

    std::shared_ptr<anychat::network::HttpClient> http_;
    std::unique_ptr<anychat::FileManagerImpl> mgr_;
};

// ---------------------------------------------------------------------------
// 1. UploadNonExistentFileReportsError
//    upload() with a path that doesn't exist should invoke the error callback;
//    it must not crash.
// ---------------------------------------------------------------------------
TEST_F(FileManagerTest, UploadNonExistentFileReportsError) {
    bool error_called = false;
    int error_code = 0;
    std::string error_message;

    mgr_->upload(
        "/nonexistent/path/file.bin",
        ANYCHAT_FILE_TYPE_FILE,
        nullptr, // no progress callback
        anychat::AnyChatValueCallback<anychat::FileInfo>{
            .on_success = [](const anychat::FileInfo&) {},
            .on_error =
                [&](int code, const std::string& error) {
                    error_called = true;
                    error_code = code;
                    error_message = error;
                },
        }
    );

    EXPECT_TRUE(error_called) << "on_error callback should be called";
    EXPECT_EQ(error_code, -1);
    EXPECT_FALSE(error_message.empty());
}

// ---------------------------------------------------------------------------
// 2. GetDownloadUrlDoesNotCrash
//    getDownloadUrl() fires an HTTP GET; no server means the callback gets
//    an error response, but no crash should occur.
// ---------------------------------------------------------------------------
TEST_F(FileManagerTest, GetDownloadUrlDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getDownloadUrl("file-id-999", makeNoopValueCallback<std::string>()));
}

// ---------------------------------------------------------------------------
// 3. GetFileInfoDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(FileManagerTest, GetFileInfoDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->getFileInfo("file-id-999", makeNoopValueCallback<anychat::FileInfo>()));
}

// ---------------------------------------------------------------------------
// 4. ListFilesDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(FileManagerTest, ListFilesDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->listFiles(ANYCHAT_FILE_TYPE_UNSPECIFIED, 1, 20, makeNoopValueCallback<anychat::FileListResult>()));
}

// ---------------------------------------------------------------------------
// 5. UploadClientLogNonExistentFileReportsError
// ---------------------------------------------------------------------------
TEST_F(FileManagerTest, UploadClientLogNonExistentFileReportsError) {
    bool error_called = false;
    int error_code = 0;
    std::string error_message;

    mgr_->uploadClientLog(
        "/nonexistent/path/client.log",
        nullptr,
        anychat::AnyChatValueCallback<anychat::FileInfo>{
            .on_success = [](const anychat::FileInfo&) {},
            .on_error =
                [&](int code, const std::string& error) {
                    error_called = true;
                    error_code = code;
                    error_message = error;
                },
        },
        24
    );

    EXPECT_TRUE(error_called) << "on_error callback should be called";
    EXPECT_EQ(error_code, -1);
    EXPECT_FALSE(error_message.empty());
}

// ---------------------------------------------------------------------------
// 6. DeleteFileDoesNotCrash
// ---------------------------------------------------------------------------
TEST_F(FileManagerTest, DeleteFileDoesNotCrash) {
    EXPECT_NO_THROW(mgr_->deleteFile("file-id-999", makeNoopCallback()));
}
