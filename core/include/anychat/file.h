#pragma once
#include "types.h"
#include <functional>
#include <string>

namespace anychat {

using FileCallback           = std::function<void(bool ok, std::string err)>;
using FileInfoCallback       = std::function<void(bool ok, FileInfo info, std::string err)>;
using UploadProgressCallback = std::function<void(int64_t uploaded, int64_t total)>;

class FileManager {
public:
    virtual ~FileManager() = default;

    // Three-step upload: get-token → PUT → complete
    // local_path: absolute path to the file to upload
    // on_progress: called periodically with bytes uploaded / total
    // on_done: called with the file_id and download_url on success
    virtual void upload(const std::string& local_path,
                        const std::string& file_type,   // "image" | "video" | "audio" | "file"
                        UploadProgressCallback on_progress,
                        FileInfoCallback on_done) = 0;

    // GET /files/{fileId}/download → presigned URL
    virtual void getDownloadUrl(const std::string& file_id,
                                std::function<void(bool ok, std::string url, std::string err)> cb) = 0;

    // DELETE /files/{fileId}
    virtual void deleteFile(const std::string& file_id, FileCallback cb) = 0;
};

} // namespace anychat
