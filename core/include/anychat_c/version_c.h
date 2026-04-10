#pragma once

#include "types_c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Callback types ---- */

typedef void (*AnyChatVersionCheckCallback)(
    void* userdata,
    int success,
    const AnyChatVersionCheckResult_C* result,
    const char* error
);

typedef void (*AnyChatVersionInfoCallback)(
    void* userdata,
    int success,
    const AnyChatVersionInfo_C* version,
    const char* error
);

typedef void (*AnyChatVersionListCallback)(void* userdata, const AnyChatVersionList_C* list, const char* error);
typedef void (*AnyChatVersionResultCallback)(void* userdata, int success, const char* error);

/* ---- Version operations ---- */

/* GET /versions/check?platform=&version=&buildNumber= */
ANYCHAT_C_API int anychat_version_check(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* version,
    int32_t build_number,
    void* userdata,
    AnyChatVersionCheckCallback callback
);

/* GET /versions/latest?platform=&releaseType= */
ANYCHAT_C_API int anychat_version_get_latest(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* release_type,
    void* userdata,
    AnyChatVersionInfoCallback callback
);

/* GET /versions/list?platform=&releaseType=&page=&pageSize= */
ANYCHAT_C_API int anychat_version_list(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* release_type,
    int page,
    int page_size,
    void* userdata,
    AnyChatVersionListCallback callback
);

/* POST /versions/report */
ANYCHAT_C_API int anychat_version_report(
    AnyChatVersionHandle handle,
    const char* platform,
    const char* version,
    int32_t build_number,
    const char* device_id,
    const char* os_version,
    const char* sdk_version,
    void* userdata,
    AnyChatVersionResultCallback callback
);

#ifdef __cplusplus
}
#endif
