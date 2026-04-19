#pragma once

#include "sdk_callbacks.h"
#include "sdk_types.h"

#include "network/http_client.h"

#include <functional>
#include <memory>
#include <string>

namespace anychat {

using UploadProgressCallback = std::function<void(int64_t uploaded, int64_t total)>;

class FileManagerImpl {
public:
    explicit FileManagerImpl(std::shared_ptr<network::HttpClient> http);

    // Three-step upload: get-token -> PUT -> complete
    // local_path: absolute path to the file to upload
    // on_progress: called periodically with bytes uploaded / total
    // on_done: called with the file_id and download_url on success
    void upload(
        const std::string& local_path,
        int32_t file_type, // ANYCHAT_FILE_TYPE_*
        UploadProgressCallback on_progress,
        AnyChatValueCallback<FileInfo> on_done
    );

    // GET /files/{fileId}/download -> presigned URL
    void getDownloadUrl(const std::string& file_id, AnyChatValueCallback<std::string> cb);

    // GET /files/{fileId}
    void getFileInfo(const std::string& file_id, AnyChatValueCallback<FileInfo> cb);

    // GET /files?fileType=&page=&pageSize=
    // file_type = 0 means no type filter.
    void listFiles(int32_t file_type, int page, int page_size, AnyChatValueCallback<FileListResult> cb);

    // Three-step client log upload: POST /logs/upload -> PUT upload_url -> POST /logs/complete
    void uploadClientLog(
        const std::string& local_path,
        UploadProgressCallback on_progress,
        AnyChatValueCallback<FileInfo> on_done,
        int32_t expires_hours = 0
    );

    // DELETE /files/{fileId}
    void deleteFile(const std::string& file_id, AnyChatCallback cb);

private:
    std::shared_ptr<network::HttpClient> http_;
};

} // namespace anychat
