#pragma once

#include "anychat/version.h"

#include "network/http_client.h"

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace anychat {

class VersionManagerImpl : public VersionManager {
public:
    explicit VersionManagerImpl(std::shared_ptr<network::HttpClient> http);

    void checkVersion(
        const std::string& platform,
        const std::string& version,
        int32_t build_number,
        CheckVersionCallback callback
    ) override;

    void getLatestVersion(
        const std::string& platform,
        const std::string& release_type,
        LatestVersionCallback callback
    ) override;

    void listVersions(
        const std::string& platform,
        const std::string& release_type,
        int page,
        int page_size,
        VersionListCallback callback
    ) override;

    void reportVersion(
        const std::string& platform,
        const std::string& version,
        int32_t build_number,
        const std::string& device_id,
        const std::string& os_version,
        const std::string& sdk_version,
        ResultCallback callback
    ) override;

private:
    static VersionUpdateInfo parseUpdateInfo(const nlohmann::json& j);
    static VersionCheckResult parseCheckResult(const nlohmann::json& j);
    static AppVersionInfo parseVersionInfo(const nlohmann::json& j);
    static int64_t parseTimestampMs(const nlohmann::json& value);
    static std::string urlEncode(const std::string& input);

    std::shared_ptr<network::HttpClient> http_;
};

} // namespace anychat
