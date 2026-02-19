#pragma once
#include "anychat/file.h"
#include "network/http_client.h"
#include <memory>
#include <mutex>

namespace anychat {

class FileManagerImpl : public FileManager {
public:
    explicit FileManagerImpl(std::shared_ptr<network::HttpClient> http);

    // FileManager interface
    void upload(const std::string& local_path,
                const std::string& file_type,
                UploadProgressCallback on_progress,
                FileInfoCallback on_done) override;

    void getDownloadUrl(const std::string& file_id,
                        std::function<void(bool ok, std::string url, std::string err)> cb) override;

    void deleteFile(const std::string& file_id, FileCallback cb) override;

private:
    std::shared_ptr<network::HttpClient> http_;
};

} // namespace anychat
