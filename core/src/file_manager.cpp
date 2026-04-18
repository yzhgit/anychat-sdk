#include "file_manager.h"

#include "json_common.h"

#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace anychat::file_manager_detail {

using json_common::ApiEnvelope;
using json_common::parseApiEnvelopeResponse;
using json_common::parseBoolValue;
using json_common::parseInt32Value;
using json_common::parseInt64Value;
using json_common::parseTimestampMs;
using json_common::writeJson;

using IntegerValue = std::variant<int64_t, double, std::string>;
using OptionalIntegerValue = std::optional<IntegerValue>;
using BooleanValue = std::variant<bool, int64_t, double, std::string>;
using OptionalBooleanValue = std::optional<BooleanValue>;

struct UploadTokenRequest {
    std::string file_name{};
    int32_t file_type = 0;
    int64_t file_size = 0;
    std::string mime_type{};
};

struct UploadClientLogInitRequest {
    std::string file_name{};
    int64_t file_size = 0;
    int32_t expires_hours = 0;
};

struct UploadLogCompleteRequest {
    std::string file_id{};
};

struct UploadTokenPayload {
    std::string file_id{};
    std::string upload_url{};
};

struct DownloadUrlPayload {
    std::string download_url{};
};

struct FileInfoPayload {
    std::string file_id{};
    std::string file_name{};
    OptionalIntegerValue file_type{};
    OptionalIntegerValue file_size{};
    std::string mime_type{};
    std::string download_url{};
    json_common::OptionalTimestampValue created_at{};
};

using FileInfoDataValue = std::variant<std::monostate, FileInfoPayload>;

struct FileListDataPayload {
    std::optional<std::vector<FileInfoPayload>> files{};
    OptionalIntegerValue total{};
};

struct DeleteFilePayload {
    OptionalBooleanValue success{};
};

constexpr int32_t kFileTypeUnspecified = 0;
constexpr int32_t kFileTypeImage = 1;
constexpr int32_t kFileTypeVideo = 2;
constexpr int32_t kFileTypeAudio = 3;
constexpr int32_t kFileTypeFile = 4;
constexpr int32_t kFileTypeLog = 5;

int32_t normalizeFileType(int32_t file_type, int32_t fallback = kFileTypeUnspecified) {
    switch (file_type) {
    case kFileTypeImage:
    case kFileTypeVideo:
    case kFileTypeAudio:
    case kFileTypeFile:
    case kFileTypeLog:
        return file_type;
    default:
        return fallback;
    }
}

std::string extractFileName(const std::string& local_path) {
    std::string file_name = local_path;
    const auto slash = file_name.find_last_of('/');
    if (slash != std::string::npos) {
        file_name = file_name.substr(slash + 1);
    }
#ifdef _WIN32
    const auto bslash = file_name.find_last_of('\\');
    if (bslash != std::string::npos) {
        file_name = file_name.substr(bslash + 1);
    }
#endif
    return file_name;
}

bool readFileBytes(const std::string& local_path, std::string& file_name, std::string& file_bytes, std::string& err) {
    file_name = extractFileName(local_path);

    std::ifstream ifs(local_path, std::ios::binary);
    if (!ifs.is_open()) {
        err = "cannot open file: " + local_path;
        return false;
    }

    file_bytes.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    if (ifs.bad()) {
        err = "read file failed: " + local_path;
        return false;
    }

    if (file_bytes.empty()) {
        err = "file is empty: " + local_path;
        return false;
    }

    return true;
}

template <typename T>
bool parseTypedDataResponse(
    const network::HttpResponse& resp,
    ApiEnvelope<T>& wrapped,
    const std::string& fallback_error = "server error"
) {
    return parseApiEnvelopeResponse(resp, wrapped, fallback_error, false, true);
}

FileInfo parseFileInfoPayload(const FileInfoPayload& payload) {
    FileInfo info;
    info.file_id = payload.file_id;
    info.file_name = payload.file_name;
    info.file_type = normalizeFileType(parseInt32Value(payload.file_type, kFileTypeUnspecified), kFileTypeUnspecified);
    info.file_size_bytes = parseInt64Value(payload.file_size, 0);
    info.mime_type = payload.mime_type;
    info.download_url = payload.download_url;
    info.created_at_ms = parseTimestampMs(payload.created_at);
    return info;
}

FileInfo parseFileInfoData(const FileInfoDataValue& data) {
    if (const auto* direct = std::get_if<FileInfoPayload>(&data); direct != nullptr) {
        return parseFileInfoPayload(*direct);
    }
    return {};
}

const std::vector<FileInfoPayload>* toFileInfoPayloadList(const FileListDataPayload& data) {
    return data.files.has_value() ? &(*data.files) : nullptr;
}

} // namespace anychat::file_manager_detail

namespace anychat {
using namespace file_manager_detail;

FileManagerImpl::FileManagerImpl(std::shared_ptr<network::HttpClient> http)
    : http_(std::move(http)) {}

void FileManagerImpl::upload(
    const std::string& local_path,
    int32_t file_type,
    UploadProgressCallback on_progress,
    AnyChatValueCallback<FileInfo> on_done
) {
    std::string file_name;
    auto file_bytes = std::make_shared<std::string>();
    std::string read_err;
    if (!readFileBytes(local_path, file_name, *file_bytes, read_err)) {
        if (on_done.on_error) {
            on_done.on_error(-1, read_err);
        }
        return;
    }

    UploadTokenRequest req_body{
        .file_name = file_name,
        .file_type = normalizeFileType(file_type),
        .file_size = static_cast<int64_t>(file_bytes->size()),
        .mime_type = "application/octet-stream",
    };

    std::string req_json;
    std::string req_err;
    if (!writeJson(req_body, req_json, req_err)) {
        if (on_done.on_error) {
            on_done.on_error(-1, req_err);
        }
        return;
    }

    http_->post(
        "/files/upload-token",
        req_json,
        [this, file_bytes, on_progress, on_done](network::HttpResponse resp) {
            ApiEnvelope<UploadTokenPayload> root{};
            if (!parseTypedDataResponse(resp, root, "upload-token failed")) {
                if (on_done.on_error) {
                    on_done.on_error(root.code, root.message);
                }
                return;
            }

            const UploadTokenPayload& token = root.data;
            if (token.file_id.empty() || token.upload_url.empty()) {
                if (on_done.on_error) {
                    on_done.on_error(-1, "upload-token response missing file id or upload url");
                }
                return;
            }

            if (on_progress) {
                on_progress(0, static_cast<int64_t>(file_bytes->size()));
            }

            http_->put(
                token.upload_url,
                *file_bytes,
                [this, file_id = token.file_id, file_bytes, on_progress, on_done](network::HttpResponse put_resp) {
                    if (!put_resp.error.empty()) {
                        if (on_done.on_error) {
                            on_done.on_error(-1, put_resp.error);
                        }
                        return;
                    }

                    if (put_resp.status_code < 200 || put_resp.status_code >= 300) {
                        if (on_done.on_error) {
                            on_done.on_error(-1, "upload PUT failed: " + put_resp.body);
                        }
                        return;
                    }

                    if (on_progress) {
                        const auto uploaded = static_cast<int64_t>(file_bytes->size());
                        on_progress(uploaded, uploaded);
                    }

                    http_->post("/files/" + file_id + "/complete", "{}", [file_id, on_done](network::HttpResponse complete_resp) {
                        ApiEnvelope<FileInfoDataValue> complete_root{};
                        if (!parseTypedDataResponse(complete_resp, complete_root, "complete failed")) {
                            if (on_done.on_error) {
                                on_done.on_error(complete_root.code, complete_root.message);
                            }
                            return;
                        }

                        FileInfo info = parseFileInfoData(complete_root.data);
                        if (info.file_id.empty()) {
                            info.file_id = file_id;
                        }

                        if (on_done.on_success) {
                            on_done.on_success(info);
                        }
                    });
                }
            );
        }
    );
}

void FileManagerImpl::getDownloadUrl(
    const std::string& file_id,
    AnyChatValueCallback<std::string> cb
) {
    http_->get("/files/" + file_id + "/download", [cb](network::HttpResponse resp) {
        ApiEnvelope<DownloadUrlPayload> root{};
        if (!parseTypedDataResponse(resp, root, "getDownloadUrl failed")) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        const DownloadUrlPayload& data = root.data;
        if (data.download_url.empty()) {
            if (cb.on_error) {
                cb.on_error(-1, "download url missing in response");
            }
            return;
        }

        if (cb.on_success) {
            cb.on_success(data.download_url);
        }
    });
}

void FileManagerImpl::getFileInfo(const std::string& file_id, AnyChatValueCallback<FileInfo> cb) {
    http_->get("/files/" + file_id, [cb, file_id](network::HttpResponse resp) {
        ApiEnvelope<FileInfoDataValue> root{};
        if (!parseTypedDataResponse(resp, root, "getFileInfo failed")) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        FileInfo info = parseFileInfoData(root.data);
        if (info.file_id.empty()) {
            info.file_id = file_id;
        }

        if (cb.on_success) {
            cb.on_success(info);
        }
    });
}

void FileManagerImpl::listFiles(
    int32_t file_type,
    int page,
    int page_size,
    AnyChatValueCallback<FileListResult> cb
) {
    const int safe_page = page > 0 ? page : 1;
    const int safe_page_size = page_size > 0 ? page_size : 20;

    std::string path =
        "/files?page=" + std::to_string(safe_page) + "&page_size=" + std::to_string(safe_page_size);
    const int32_t normalized_file_type = normalizeFileType(file_type, kFileTypeUnspecified);
    if (normalized_file_type != kFileTypeUnspecified) {
        path += "&file_type=" + std::to_string(normalized_file_type);
    }

    http_->get(path, [cb, safe_page, safe_page_size](network::HttpResponse resp) {
        ApiEnvelope<FileListDataPayload> root{};
        if (!parseTypedDataResponse(resp, root, "listFiles failed")) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        FileListResult result;
        const auto* payloads = toFileInfoPayloadList(root.data);
        if (payloads != nullptr) {
            result.files.reserve(payloads->size());
            for (const auto& item : *payloads) {
                result.files.push_back(parseFileInfoPayload(item));
            }
        }

        result.total = parseInt64Value(root.data.total, 0);
        if (result.total == 0 && !result.files.empty()) {
            result.total = static_cast<int64_t>(result.files.size());
        }
        result.page = safe_page;
        result.page_size = safe_page_size;

        if (cb.on_success) {
            cb.on_success(result);
        }
    });
}

void FileManagerImpl::uploadClientLog(
    const std::string& local_path,
    UploadProgressCallback on_progress,
    AnyChatValueCallback<FileInfo> on_done,
    int32_t expires_hours
) {
    std::string file_name;
    auto file_bytes = std::make_shared<std::string>();
    std::string read_err;
    if (!readFileBytes(local_path, file_name, *file_bytes, read_err)) {
        if (on_done.on_error) {
            on_done.on_error(-1, read_err);
        }
        return;
    }

    UploadClientLogInitRequest req_body{
        .file_name = file_name,
        .file_size = static_cast<int64_t>(file_bytes->size()),
        .expires_hours = expires_hours < 0 ? 0 : expires_hours,
    };

    std::string req_json;
    std::string req_err;
    if (!writeJson(req_body, req_json, req_err)) {
        if (on_done.on_error) {
            on_done.on_error(-1, req_err);
        }
        return;
    }

    http_->post(
        "/logs/upload",
        req_json,
        [this, file_bytes, on_progress, on_done](network::HttpResponse resp) {
            ApiEnvelope<UploadTokenPayload> root{};
            if (!parseTypedDataResponse(resp, root, "log upload init failed")) {
                if (on_done.on_error) {
                    on_done.on_error(root.code, root.message);
                }
                return;
            }

            const UploadTokenPayload& init = root.data;
            if (init.file_id.empty() || init.upload_url.empty()) {
                if (on_done.on_error) {
                    on_done.on_error(-1, "log upload init response missing file id or upload url");
                }
                return;
            }

            if (on_progress) {
                on_progress(0, static_cast<int64_t>(file_bytes->size()));
            }

            http_->put(
                init.upload_url,
                *file_bytes,
                [this, file_id = init.file_id, file_bytes, on_progress, on_done](network::HttpResponse put_resp) {
                    if (!put_resp.error.empty()) {
                        if (on_done.on_error) {
                            on_done.on_error(-1, put_resp.error);
                        }
                        return;
                    }

                    if (put_resp.status_code < 200 || put_resp.status_code >= 300) {
                        if (on_done.on_error) {
                            on_done.on_error(-1, "log upload PUT failed: " + put_resp.body);
                        }
                        return;
                    }

                    if (on_progress) {
                        const auto uploaded = static_cast<int64_t>(file_bytes->size());
                        on_progress(uploaded, uploaded);
                    }

                    const UploadLogCompleteRequest complete_body{.file_id = file_id};
                    std::string complete_body_json;
                    std::string complete_body_err;
                    if (!writeJson(complete_body, complete_body_json, complete_body_err)) {
                        if (on_done.on_error) {
                            on_done.on_error(-1, complete_body_err);
                        }
                        return;
                    }

                    http_->post("/logs/complete", complete_body_json, [file_id, on_done](network::HttpResponse complete_resp) {
                        ApiEnvelope<FileInfoDataValue> complete_root{};
                        if (!parseTypedDataResponse(complete_resp, complete_root, "log upload complete failed")) {
                            if (on_done.on_error) {
                                on_done.on_error(complete_root.code, complete_root.message);
                            }
                            return;
                        }

                        FileInfo info = parseFileInfoData(complete_root.data);
                        if (info.file_id.empty()) {
                            info.file_id = file_id;
                        }
                        if (info.file_type == kFileTypeUnspecified) {
                            info.file_type = kFileTypeLog;
                        }

                        if (on_done.on_success) {
                            on_done.on_success(info);
                        }
                    });
                }
            );
        }
    );
}

void FileManagerImpl::deleteFile(const std::string& file_id, AnyChatCallback cb) {
    http_->del("/files/" + file_id, [cb](network::HttpResponse resp) {
        ApiEnvelope<DeleteFilePayload> root{};
        if (!parseTypedDataResponse(resp, root, "deleteFile failed")) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        if (root.data.success.has_value() && !parseBoolValue(root.data.success, true)) {
            if (cb.on_error) {
                cb.on_error(-1, "deleteFile failed");
            }
            return;
        }

        if (cb.on_success) {
            cb.on_success();
        }
    });
}

} // namespace anychat
