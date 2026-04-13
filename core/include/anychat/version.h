#pragma once

#include "callbacks.h"
#include "types.h"

#include <string>

namespace anychat {

class VersionManager {
public:
    virtual ~VersionManager() = default;

    // GET /versions/check?platform=&version=&buildNumber=
    virtual void checkVersion(
        const std::string& platform,
        const std::string& version,
        int32_t build_number,
        AnyChatValueCallback<VersionCheckResult> callback
    ) = 0;

    // GET /versions/latest?platform=&releaseType=
    virtual void getLatestVersion(
        const std::string& platform,
        const std::string& release_type,
        AnyChatValueCallback<AppVersionInfo> callback
    ) = 0;

    // GET /versions/list?platform=&releaseType=&page=&pageSize=
    virtual void listVersions(
        const std::string& platform,
        const std::string& release_type,
        int page,
        int page_size,
        AnyChatValueCallback<VersionListResult> callback
    ) = 0;

    // POST /versions/report
    virtual void reportVersion(
        const std::string& platform,
        const std::string& version,
        int32_t build_number,
        const std::string& device_id,
        const std::string& os_version,
        const std::string& sdk_version,
        AnyChatCallback callback
    ) = 0;
};

} // namespace anychat
