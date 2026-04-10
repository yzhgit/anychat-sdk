#include "anychat_c/version_c.h"

#include "handles_c.h"
#include "utils_c.h"

#include <cstdlib>
#include <vector>

namespace {

void updateInfoToC(const anychat::VersionUpdateInfo& src, AnyChatVersionUpdateInfo_C* dst) {
    anychat_strlcpy(dst->title, src.title.c_str(), sizeof(dst->title));
    anychat_strlcpy(dst->content, src.content.c_str(), sizeof(dst->content));
    anychat_strlcpy(dst->download_url, src.download_url.c_str(), sizeof(dst->download_url));
    dst->file_size = src.file_size;
    anychat_strlcpy(dst->file_hash, src.file_hash.c_str(), sizeof(dst->file_hash));
}

void checkResultToC(const anychat::VersionCheckResult& src, AnyChatVersionCheckResult_C* dst) {
    dst->has_update = src.has_update ? 1 : 0;
    anychat_strlcpy(dst->latest_version, src.latest_version.c_str(), sizeof(dst->latest_version));
    dst->latest_build_number = src.latest_build_number;
    dst->force_update = src.force_update ? 1 : 0;
    anychat_strlcpy(dst->min_version, src.min_version.c_str(), sizeof(dst->min_version));
    dst->min_build_number = src.min_build_number;
    updateInfoToC(src.update_info, &dst->update_info);
}

void versionInfoToC(const anychat::AppVersionInfo& src, AnyChatVersionInfo_C* dst) {
    dst->id = src.id;
    anychat_strlcpy(dst->platform, src.platform.c_str(), sizeof(dst->platform));
    anychat_strlcpy(dst->version, src.version.c_str(), sizeof(dst->version));
    dst->build_number = src.build_number;
    dst->version_code = src.version_code;
    anychat_strlcpy(dst->min_version, src.min_version.c_str(), sizeof(dst->min_version));
    dst->min_build_number = src.min_build_number;
    dst->force_update = src.force_update ? 1 : 0;
    anychat_strlcpy(dst->release_type, src.release_type.c_str(), sizeof(dst->release_type));
    anychat_strlcpy(dst->title, src.title.c_str(), sizeof(dst->title));
    anychat_strlcpy(dst->content, src.content.c_str(), sizeof(dst->content));
    anychat_strlcpy(dst->download_url, src.download_url.c_str(), sizeof(dst->download_url));
    dst->file_size = src.file_size;
    anychat_strlcpy(dst->file_hash, src.file_hash.c_str(), sizeof(dst->file_hash));
    dst->published_at_ms = src.published_at_ms;
}

} // namespace

extern "C" {

int anychat_version_check(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* version,
    int32_t build_number,
    void* userdata,
    AnyChatVersionCheckCallback callback
) {
    if (!handle || !handle->impl || !platform || !version) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }

    handle->impl->checkVersion(
        platform,
        version,
        build_number,
        [userdata, callback](bool ok, const anychat::VersionCheckResult& result, const std::string& err) {
            if (!callback)
                return;

            if (ok) {
                AnyChatVersionCheckResult_C c_result{};
                checkResultToC(result, &c_result);
                callback(userdata, 1, &c_result, "");
            } else {
                callback(userdata, 0, nullptr, err.c_str());
            }
        }
    );

    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_version_get_latest(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* release_type,
    void* userdata,
    AnyChatVersionInfoCallback callback
) {
    if (!handle || !handle->impl || !platform) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }

    handle->impl->getLatestVersion(
        platform,
        release_type ? release_type : "",
        [userdata, callback](bool ok, const anychat::AppVersionInfo& version, const std::string& err) {
            if (!callback)
                return;

            if (ok) {
                AnyChatVersionInfo_C c_version{};
                versionInfoToC(version, &c_version);
                callback(userdata, 1, &c_version, "");
            } else {
                callback(userdata, 0, nullptr, err.c_str());
            }
        }
    );

    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_version_list(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* release_type,
    int page,
    int page_size,
    void* userdata,
    AnyChatVersionListCallback callback
) {
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }

    handle->impl->listVersions(
        platform ? platform : "",
        release_type ? release_type : "",
        page,
        page_size,
        [userdata, callback, page, page_size](
            const std::vector<anychat::AppVersionInfo>& versions,
            int64_t total,
            const std::string& err
        ) {
            if (!callback)
                return;

            const int count = static_cast<int>(versions.size());
            AnyChatVersionList_C c_list{};
            c_list.count = count;
            c_list.total = total;
            c_list.page = page;
            c_list.page_size = page_size;
            c_list.items = count > 0
                               ? static_cast<AnyChatVersionInfo_C*>(std::calloc(count, sizeof(AnyChatVersionInfo_C)))
                               : nullptr;

            for (int i = 0; i < count; ++i) {
                versionInfoToC(versions[i], &c_list.items[i]);
            }

            callback(userdata, &c_list, err.empty() ? nullptr : err.c_str());
            std::free(c_list.items);
        }
    );

    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_version_report(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* version,
    int32_t build_number,
    const char* device_id,
    const char* os_version,
    const char* sdk_version,
    void* userdata,
    AnyChatVersionResultCallback callback
) {
    if (!handle || !handle->impl || !platform || !version) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }

    handle->impl->reportVersion(
        platform,
        version,
        build_number,
        device_id ? device_id : "",
        os_version ? os_version : "",
        sdk_version ? sdk_version : "",
        [userdata, callback](bool ok, const std::string& err) {
            if (callback)
                callback(userdata, ok ? 1 : 0, err.c_str());
        }
    );

    anychat_clear_last_error();
    return ANYCHAT_OK;
}

} // extern "C"
