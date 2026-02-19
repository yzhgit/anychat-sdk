#include "handles_c.h"
#include "anychat_c/file_c.h"
#include "utils_c.h"

#include <cstring>

namespace {

void fileInfoToC(const anychat::FileInfo& src, AnyChatFileInfo_C* dst) {
    anychat_strlcpy(dst->file_id,      src.file_id.c_str(),      sizeof(dst->file_id));
    anychat_strlcpy(dst->file_name,    src.file_name.c_str(),    sizeof(dst->file_name));
    anychat_strlcpy(dst->file_type,    src.file_type.c_str(),    sizeof(dst->file_type));
    anychat_strlcpy(dst->mime_type,    src.mime_type.c_str(),    sizeof(dst->mime_type));
    anychat_strlcpy(dst->download_url, src.download_url.c_str(), sizeof(dst->download_url));
    dst->file_size_bytes = src.file_size_bytes;
    dst->created_at_ms   = src.created_at_ms;
}

} // namespace

extern "C" {

int anychat_file_upload(
    AnyChatFileHandle             handle,
    const char*                   local_path,
    const char*                   file_type,
    void*                         userdata,
    AnyChatUploadProgressCallback on_progress,
    AnyChatFileInfoCallback       on_done)
{
    if (!handle || !handle->impl || !local_path || !file_type) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }

    handle->impl->upload(
        local_path, file_type,
        [userdata, on_progress](int64_t uploaded, int64_t total) {
            if (on_progress) on_progress(userdata, uploaded, total);
        },
        [userdata, on_done](bool ok, anychat::FileInfo info, std::string err) {
            if (!on_done) return;
            if (ok) {
                AnyChatFileInfo_C c_info{};
                fileInfoToC(info, &c_info);
                on_done(userdata, 1, &c_info, "");
            } else {
                on_done(userdata, 0, nullptr, err.c_str());
            }
        });

    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_file_get_download_url(
    AnyChatFileHandle          handle,
    const char*                file_id,
    void*                      userdata,
    AnyChatDownloadUrlCallback callback)
{
    if (!handle || !handle->impl || !file_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->getDownloadUrl(file_id,
        [userdata, callback](bool ok, std::string url, std::string err) {
            if (callback)
                callback(userdata, ok ? 1 : 0,
                         ok ? url.c_str() : nullptr,
                         err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_file_delete(
    AnyChatFileHandle   handle,
    const char*         file_id,
    void*               userdata,
    AnyChatFileCallback callback)
{
    if (!handle || !handle->impl || !file_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->deleteFile(file_id,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

} // extern "C"
