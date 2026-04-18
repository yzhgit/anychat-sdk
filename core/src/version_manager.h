#pragma once

#include "internal/version.h"

#include "network/http_client.h"

#include <memory>
#include <string>

namespace anychat {

class VersionManagerImpl : public VersionManager {
public:
    explicit VersionManagerImpl(std::shared_ptr<network::HttpClient> http);

    void checkVersion(
        int32_t platform,
        const std::string& version,
        int32_t build_number,
        AnyChatValueCallback<VersionCheckResult> callback
    ) override;

    void getLatestVersion(
        int32_t platform,
        int32_t release_type,
        AnyChatValueCallback<AppVersionInfo> callback
    ) override;

    void listVersions(
        int32_t platform,
        int32_t release_type,
        int page,
        int page_size,
        AnyChatValueCallback<VersionListResult> callback
    ) override;

    void reportVersion(
        int32_t platform,
        const std::string& version,
        int32_t build_number,
        const std::string& device_id,
        const std::string& os_version,
        const std::string& sdk_version,
        AnyChatCallback callback
    ) override;

private:
    static std::string urlEncode(const std::string& input);

    std::shared_ptr<network::HttpClient> http_;
};

} // namespace anychat
