#include "file_manager.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace anychat {

FileManagerImpl::FileManagerImpl(std::shared_ptr<network::HttpClient> http)
    : http_(std::move(http))
{}

void FileManagerImpl::upload(const std::string& local_path,
                              const std::string& file_type,
                              UploadProgressCallback on_progress,
                              FileInfoCallback on_done)
{
    // Extract file name from path
    std::string file_name = local_path;
    auto slash = local_path.rfind('/');
    if (slash != std::string::npos) file_name = local_path.substr(slash + 1);
#ifdef _WIN32
    auto bslash = file_name.rfind('\\');
    if (bslash != std::string::npos) file_name = file_name.substr(bslash + 1);
#endif

    // Step 1: POST /files/upload-token to get upload URL and file_id
    nlohmann::json req_body = {
        {"fileName",  file_name},
        {"fileType",  file_type},
        {"fileSize",  0},
        {"mimeType",  "application/octet-stream"}
    };

    http_->post("/files/upload-token", req_body.dump(),
        [this, local_path, on_progress, on_done](network::HttpResponse resp) {
            if (!resp.error.empty()) {
                if (on_done) on_done(false, FileInfo{}, resp.error);
                return;
            }
            if (resp.status_code < 200 || resp.status_code >= 300) {
                if (on_done) on_done(false, FileInfo{}, "upload-token failed: " + resp.body);
                return;
            }

            std::string file_id;
            std::string upload_url;
            try {
                auto j = nlohmann::json::parse(resp.body);
                auto& data = j.at("data");
                file_id    = data.at("fileId").get<std::string>();
                upload_url = data.at("uploadUrl").get<std::string>();
            } catch (const std::exception& e) {
                if (on_done) on_done(false, FileInfo{}, std::string("parse error: ") + e.what());
                return;
            }

            // Step 2: Read file from disk
            std::ifstream ifs(local_path, std::ios::binary);
            if (!ifs.is_open()) {
                if (on_done) on_done(false, FileInfo{}, "cannot open file: " + local_path);
                return;
            }
            std::string file_bytes(
                (std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());
            ifs.close();

            // Notify progress: starting upload (0 / total)
            if (on_progress) on_progress(0, static_cast<int64_t>(file_bytes.size()));

            // Step 2b: PUT file bytes to presigned upload URL
            // upload_url may be an absolute URL (e.g. MinIO presigned URL);
            // pass it directly – HttpClient uses it as the path/url.
            http_->put(upload_url, file_bytes,
                [this, file_id, file_bytes_size = file_bytes.size(), on_progress, on_done]
                (network::HttpResponse put_resp) {
                    if (!put_resp.error.empty()) {
                        if (on_done) on_done(false, FileInfo{}, put_resp.error);
                        return;
                    }
                    if (put_resp.status_code < 200 || put_resp.status_code >= 300) {
                        if (on_done) on_done(false, FileInfo{}, "PUT failed: " + put_resp.body);
                        return;
                    }

                    // Notify progress: upload complete
                    if (on_progress) {
                        on_progress(static_cast<int64_t>(file_bytes_size),
                                    static_cast<int64_t>(file_bytes_size));
                    }

                    // Step 3: POST /files/{file_id}/complete to activate the file
                    std::string complete_path = "/files/" + file_id + "/complete";
                    http_->post(complete_path, "{}",
                        [on_done](network::HttpResponse complete_resp) {
                            if (!complete_resp.error.empty()) {
                                if (on_done) on_done(false, FileInfo{}, complete_resp.error);
                                return;
                            }
                            if (complete_resp.status_code < 200 || complete_resp.status_code >= 300) {
                                if (on_done) on_done(false, FileInfo{}, "complete failed: " + complete_resp.body);
                                return;
                            }

                            FileInfo info;
                            try {
                                auto j = nlohmann::json::parse(complete_resp.body);
                                auto& data = j.at("data");
                                if (data.contains("fileId"))
                                    info.file_id = data["fileId"].get<std::string>();
                                if (data.contains("fileName"))
                                    info.file_name = data["fileName"].get<std::string>();
                                if (data.contains("fileType"))
                                    info.file_type = data["fileType"].get<std::string>();
                                if (data.contains("fileSize"))
                                    info.file_size_bytes = data["fileSize"].get<int64_t>();
                                if (data.contains("mimeType"))
                                    info.mime_type = data["mimeType"].get<std::string>();
                                if (data.contains("downloadUrl"))
                                    info.download_url = data["downloadUrl"].get<std::string>();
                                if (data.contains("createdAt"))
                                    info.created_at_ms = data["createdAt"].get<int64_t>();
                            } catch (...) {
                                // Partial parse failure is non-fatal; return what we have
                            }

                            if (on_done) on_done(true, info, "");
                        });
                });
        });
}

void FileManagerImpl::getDownloadUrl(const std::string& file_id,
                                     std::function<void(bool ok, std::string url, std::string err)> cb)
{
    std::string path = "/files/" + file_id + "/download";
    http_->get(path, [cb](network::HttpResponse resp) {
        if (!resp.error.empty()) {
            if (cb) cb(false, "", resp.error);
            return;
        }
        if (resp.status_code < 200 || resp.status_code >= 300) {
            if (cb) cb(false, "", "getDownloadUrl failed: " + resp.body);
            return;
        }

        std::string url;
        try {
            auto j = nlohmann::json::parse(resp.body);
            auto& data = j.at("data");
            if (data.contains("downloadUrl"))
                url = data["downloadUrl"].get<std::string>();
        } catch (const std::exception& e) {
            if (cb) cb(false, "", std::string("parse error: ") + e.what());
            return;
        }

        if (cb) cb(true, url, "");
    });
}

void FileManagerImpl::deleteFile(const std::string& file_id, FileCallback cb)
{
    std::string path = "/files/" + file_id;
    http_->del(path, [cb](network::HttpResponse resp) {
        if (!resp.error.empty()) {
            if (cb) cb(false, resp.error);
            return;
        }
        if (resp.status_code < 200 || resp.status_code >= 300) {
            if (cb) cb(false, "deleteFile failed: " + resp.body);
            return;
        }
        if (cb) cb(true, "");
    });
}

} // namespace anychat
