#pragma once

#include "sdk_callbacks.h"
#include "sdk_types.h"

#include "network/http_client.h"

#include <memory>
#include <string>

namespace anychat {

class VersionManagerImpl {
public:
    explicit VersionManagerImpl(std::shared_ptr<network::HttpClient> http);

    // GET /versions/check?platform=&version=&buildNumber=
    void checkVersion(
        int32_t platform,
        const std::string& version,
        int32_t build_number,
        AnyChatValueCallback<VersionCheckResult> callback
    );

    // GET /versions/latest?platform=&releaseType=
    void getLatestVersion(
        int32_t platform,
        int32_t release_type,
        AnyChatValueCallback<AppVersionInfo> callback
    );

    // GET /versions/list?platform=&releaseType=&page=&pageSize=
    void listVersions(
        int32_t platform,
        int32_t release_type,
        int page,
        int page_size,
        AnyChatValueCallback<VersionListResult> callback
    );

    // POST /versions/report
    void reportVersion(
        int32_t platform,
        const std::string& version,
        int32_t build_number,
        const std::string& device_id,
        const std::string& os_version,
        const std::string& sdk_version,
        AnyChatCallback callback
    );

private:
    static std::string urlEncode(const std::string& input);

    std::shared_ptr<network::HttpClient> http_;
};

} // namespace anychat
