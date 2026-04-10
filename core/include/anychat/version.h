#pragma once

#include "types.h"

#include <functional>
#include <string>
#include <vector>

namespace anychat {

class VersionManager {
public:
    using CheckVersionCallback = std::function<void(bool ok, const VersionCheckResult&, const std::string& err)>;
    using LatestVersionCallback = std::function<void(bool ok, const AppVersionInfo&, const std::string& err)>;
    using VersionListCallback =
        std::function<void(const std::vector<AppVersionInfo>& versions, int64_t total, const std::string& err)>;
    using ResultCallback = std::function<void(bool ok, const std::string& err)>;

    virtual ~VersionManager() = default;

    // GET /versions/check?platform=&version=&buildNumber=
    virtual void checkVersion(
        const std::string& platform,
        const std::string& version,
        int32_t build_number,
        CheckVersionCallback callback
    ) = 0;

    // GET /versions/latest?platform=&releaseType=
    virtual void getLatestVersion(
        const std::string& platform,
        const std::string& release_type,
        LatestVersionCallback callback
    ) = 0;

    // GET /versions/list?platform=&releaseType=&page=&pageSize=
    virtual void listVersions(
        const std::string& platform,
        const std::string& release_type,
        int page,
        int page_size,
        VersionListCallback callback
    ) = 0;

    // POST /versions/report
    virtual void reportVersion(
        const std::string& platform,
        const std::string& version,
        int32_t build_number,
        const std::string& device_id,
        const std::string& os_version,
        const std::string& sdk_version,
        ResultCallback callback
    ) = 0;
};

} // namespace anychat
