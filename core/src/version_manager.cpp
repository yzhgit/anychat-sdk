#include "version_manager.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <exception>
#include <iomanip>
#include <initializer_list>
#include <sstream>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

namespace anychat {
namespace {

const nlohmann::json* findField(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    if (!obj.is_object()) {
        return nullptr;
    }

    for (const char* key : keys) {
        auto it = obj.find(key);
        if (it != obj.end()) {
            return &(*it);
        }
    }

    return nullptr;
}

int64_t normalizeUnixEpochMs(int64_t raw) {
    // Server timestamps are usually Unix seconds; preserve millisecond inputs.
    return (raw > 0 && raw < 1'000'000'000'000LL) ? raw * 1000LL : raw;
}

time_t utcTimeFromTm(std::tm* tm_utc) {
#if defined(_WIN32)
    return _mkgmtime(tm_utc);
#else
    return timegm(tm_utc);
#endif
}

int parseTwoDigits(const std::string& text, size_t pos) {
    if (pos + 2 > text.size() || !std::isdigit(static_cast<unsigned char>(text[pos]))
        || !std::isdigit(static_cast<unsigned char>(text[pos + 1]))) {
        return -1;
    }
    return (text[pos] - '0') * 10 + (text[pos + 1] - '0');
}

std::string jsonString(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    const auto* value = findField(obj, keys);
    if (value == nullptr || value->is_null()) {
        return "";
    }

    if (value->is_string()) {
        return value->get<std::string>();
    }

    if (value->is_boolean()) {
        return value->get<bool>() ? "true" : "false";
    }

    if (value->is_number_integer()) {
        return std::to_string(value->get<int64_t>());
    }
    if (value->is_number_unsigned()) {
        return std::to_string(value->get<uint64_t>());
    }
    if (value->is_number_float()) {
        return std::to_string(value->get<double>());
    }

    return "";
}

int64_t jsonInt64(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    const auto* value = findField(obj, keys);
    if (value == nullptr || value->is_null()) {
        return 0;
    }

    if (value->is_number_integer()) {
        return value->get<int64_t>();
    }
    if (value->is_number_unsigned()) {
        return static_cast<int64_t>(value->get<uint64_t>());
    }
    if (value->is_number_float()) {
        return static_cast<int64_t>(value->get<double>());
    }
    if (value->is_string()) {
        try {
            return std::stoll(value->get<std::string>());
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

bool jsonBool(const nlohmann::json& obj, std::initializer_list<const char*> keys) {
    const auto* value = findField(obj, keys);
    if (value == nullptr || value->is_null()) {
        return false;
    }

    if (value->is_boolean()) {
        return value->get<bool>();
    }

    if (value->is_number_integer() || value->is_number_unsigned()) {
        return jsonInt64(obj, keys) != 0;
    }

    if (!value->is_string()) {
        return false;
    }

    std::string text = value->get<std::string>();
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text == "1" || text == "true" || text == "yes" || text == "on";
}

template <typename Callback>
void callbackWithParseError(const Callback& callback, const std::exception& e) {
    callback(false, {}, std::string("parse error: ") + e.what());
}

} // namespace

VersionManagerImpl::VersionManagerImpl(std::shared_ptr<network::HttpClient> http)
    : http_(std::move(http)) {}

/*static*/ int64_t VersionManagerImpl::parseTimestampMs(const nlohmann::json& value) {
    if (value.is_null()) {
        return 0;
    }

    if (value.is_number_integer()) {
        return normalizeUnixEpochMs(value.get<int64_t>());
    }
    if (value.is_number_unsigned()) {
        return normalizeUnixEpochMs(static_cast<int64_t>(value.get<uint64_t>()));
    }

    if (!value.is_string()) {
        return 0;
    }

    const std::string text = value.get<std::string>();
    if (text.empty()) {
        return 0;
    }

    const bool all_digits = std::all_of(text.begin(), text.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
    if (all_digits) {
        try {
            return normalizeUnixEpochMs(std::stoll(text));
        } catch (...) {
            return 0;
        }
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (std::sscanf(text.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6
        && std::sscanf(text.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return 0;
    }

    std::tm tm_utc{};
    tm_utc.tm_year = year - 1900;
    tm_utc.tm_mon = month - 1;
    tm_utc.tm_mday = day;
    tm_utc.tm_hour = hour;
    tm_utc.tm_min = minute;
    tm_utc.tm_sec = second;

    time_t epoch_seconds = utcTimeFromTm(&tm_utc);
    if (epoch_seconds == static_cast<time_t>(-1)) {
        return 0;
    }

    int ms = 0;
    const size_t dot_pos = text.find('.', 19);
    if (dot_pos != std::string::npos) {
        int scale = 100;
        for (size_t i = dot_pos + 1; i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) && scale > 0;
             ++i) {
            ms += (text[i] - '0') * scale;
            scale /= 10;
        }
    }

    int tz_offset_seconds = 0;
    const size_t tz_pos = text.find_first_of("+-Z", 19);
    if (tz_pos != std::string::npos && text[tz_pos] != 'Z') {
        const int hh = parseTwoDigits(text, tz_pos + 1);
        if (hh >= 0) {
            int mm = 0;
            if (tz_pos + 3 < text.size() && text[tz_pos + 3] == ':') {
                const int parsed = parseTwoDigits(text, tz_pos + 4);
                if (parsed >= 0) {
                    mm = parsed;
                }
            } else if (tz_pos + 3 < text.size() && std::isdigit(static_cast<unsigned char>(text[tz_pos + 3]))) {
                const int parsed = parseTwoDigits(text, tz_pos + 3);
                if (parsed >= 0) {
                    mm = parsed;
                }
            }

            tz_offset_seconds = hh * 3600 + mm * 60;
            if (text[tz_pos] == '-') {
                tz_offset_seconds = -tz_offset_seconds;
            }
        }
    }

    return static_cast<int64_t>(epoch_seconds - tz_offset_seconds) * 1000LL + ms;
}

/*static*/ std::string VersionManagerImpl::urlEncode(const std::string& input) {
    std::ostringstream out;
    out << std::uppercase << std::hex;
    for (unsigned char ch : input) {
        if (std::isalnum(ch) != 0 || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            out << static_cast<char>(ch);
        } else {
            out << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
        }
    }
    return out.str();
}

/*static*/ VersionUpdateInfo VersionManagerImpl::parseUpdateInfo(const nlohmann::json& j) {
    VersionUpdateInfo info;
    info.title = jsonString(j, { "title" });
    info.content = jsonString(j, { "content" });
    info.download_url = jsonString(j, { "downloadUrl", "download_url" });
    info.file_size = jsonInt64(j, { "fileSize", "file_size" });
    info.file_hash = jsonString(j, { "fileHash", "file_hash" });
    return info;
}

/*static*/ VersionCheckResult VersionManagerImpl::parseCheckResult(const nlohmann::json& j) {
    VersionCheckResult result;
    result.has_update = jsonBool(j, { "hasUpdate", "has_update" });
    result.latest_version = jsonString(j, { "latestVersion", "latest_version" });
    result.latest_build_number = static_cast<int32_t>(jsonInt64(j, { "latestBuildNumber", "latest_build_number" }));
    result.force_update = jsonBool(j, { "forceUpdate", "force_update" });
    result.min_version = jsonString(j, { "minVersion", "min_version" });
    result.min_build_number = static_cast<int32_t>(jsonInt64(j, { "minBuildNumber", "min_build_number" }));

    if (const auto* update_info = findField(j, { "updateInfo", "update_info" }); update_info != nullptr
        && update_info->is_object()) {
        result.update_info = parseUpdateInfo(*update_info);
    }
    return result;
}

/*static*/ AppVersionInfo VersionManagerImpl::parseVersionInfo(const nlohmann::json& j) {
    AppVersionInfo info;
    info.id = jsonInt64(j, { "id" });
    info.platform = jsonString(j, { "platform" });
    info.version = jsonString(j, { "version" });
    info.build_number = static_cast<int32_t>(jsonInt64(j, { "buildNumber", "build_number" }));
    info.version_code = static_cast<int32_t>(jsonInt64(j, { "versionCode", "version_code" }));
    info.min_version = jsonString(j, { "minVersion", "min_version" });
    info.min_build_number = static_cast<int32_t>(jsonInt64(j, { "minBuildNumber", "min_build_number" }));
    info.force_update = jsonBool(j, { "forceUpdate", "force_update" });
    info.release_type = jsonString(j, { "releaseType", "release_type" });
    info.title = jsonString(j, { "title" });
    info.content = jsonString(j, { "content" });
    info.download_url = jsonString(j, { "downloadUrl", "download_url" });
    info.file_size = jsonInt64(j, { "fileSize", "file_size" });
    info.file_hash = jsonString(j, { "fileHash", "file_hash" });

    if (const auto* published_at = findField(j, { "publishedAt", "published_at" }); published_at != nullptr) {
        info.published_at_ms = parseTimestampMs(*published_at);
    }
    return info;
}

void VersionManagerImpl::checkVersion(
    const std::string& platform,
    const std::string& version,
    int32_t build_number,
    CheckVersionCallback callback
) {
    if (platform.empty() || version.empty()) {
        callback(false, {}, "platform and version are required");
        return;
    }

    std::string path = "/versions/check?platform=" + urlEncode(platform) + "&version=" + urlEncode(version);
    if (build_number > 0) {
        path += "&buildNumber=" + std::to_string(build_number);
    }

    http_->get(path, [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) {
            cb(false, {}, resp.error);
            return;
        }

        try {
            const auto root = nlohmann::json::parse(resp.body);
            if (root.value("code", -1) != 0) {
                cb(false, {}, root.value("message", "server error"));
                return;
            }

            const nlohmann::json data = root.contains("data") ? root["data"] : nlohmann::json::object();
            cb(true, parseCheckResult(data), "");
        } catch (const std::exception& e) {
            callbackWithParseError(cb, e);
        }
    });
}

void VersionManagerImpl::getLatestVersion(
    const std::string& platform,
    const std::string& release_type,
    LatestVersionCallback callback
) {
    if (platform.empty()) {
        callback(false, {}, "platform is required");
        return;
    }

    std::string path = "/versions/latest?platform=" + urlEncode(platform);
    if (!release_type.empty()) {
        path += "&releaseType=" + urlEncode(release_type);
    }

    http_->get(path, [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) {
            cb(false, {}, resp.error);
            return;
        }

        try {
            const auto root = nlohmann::json::parse(resp.body);
            if (root.value("code", -1) != 0) {
                cb(false, {}, root.value("message", "server error"));
                return;
            }

            AppVersionInfo version;
            if (root.contains("data") && root["data"].is_object()) {
                const auto& data = root["data"];
                const auto* version_obj = findField(data, { "version" });
                if (version_obj != nullptr && version_obj->is_object()) {
                    version = parseVersionInfo(*version_obj);
                } else {
                    version = parseVersionInfo(data);
                }
            }

            cb(true, version, "");
        } catch (const std::exception& e) {
            callbackWithParseError(cb, e);
        }
    });
}

void VersionManagerImpl::listVersions(
    const std::string& platform,
    const std::string& release_type,
    int page,
    int page_size,
    VersionListCallback callback
) {
    if (page < 1) {
        page = 1;
    }
    if (page_size < 1) {
        page_size = 20;
    }

    std::string path = "/versions/list?page=" + std::to_string(page) + "&pageSize=" + std::to_string(page_size);
    if (!platform.empty()) {
        path += "&platform=" + urlEncode(platform);
    }
    if (!release_type.empty()) {
        path += "&releaseType=" + urlEncode(release_type);
    }

    http_->get(path, [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) {
            cb({}, 0, resp.error);
            return;
        }

        try {
            const auto root = nlohmann::json::parse(resp.body);
            if (root.value("code", -1) != 0) {
                cb({}, 0, root.value("message", "server error"));
                return;
            }

            std::vector<AppVersionInfo> versions;
            int64_t total = 0;
            if (root.contains("data") && root["data"].is_object()) {
                const auto& data = root["data"];
                total = jsonInt64(data, { "total" });
                if (const auto* arr = findField(data, { "versions" }); arr != nullptr && arr->is_array()) {
                    versions.reserve(arr->size());
                    for (const auto& item : *arr) {
                        if (item.is_object()) {
                            versions.push_back(parseVersionInfo(item));
                        }
                    }
                }
            }

            cb(versions, total, "");
        } catch (const std::exception& e) {
            cb({}, 0, std::string("parse error: ") + e.what());
        }
    });
}

void VersionManagerImpl::reportVersion(
    const std::string& platform,
    const std::string& version,
    int32_t build_number,
    const std::string& device_id,
    const std::string& os_version,
    const std::string& sdk_version,
    ResultCallback callback
) {
    if (platform.empty() || version.empty()) {
        callback(false, "platform and version are required");
        return;
    }

    nlohmann::json body;
    body["platform"] = platform;
    body["version"] = version;
    if (build_number > 0) {
        body["buildNumber"] = build_number;
    }
    if (!device_id.empty()) {
        body["deviceId"] = device_id;
    }
    if (!os_version.empty()) {
        body["osVersion"] = os_version;
    }
    if (!sdk_version.empty()) {
        body["sdkVersion"] = sdk_version;
    }

    http_->post("/versions/report", body.dump(), [cb = std::move(callback)](network::HttpResponse resp) {
        if (!resp.error.empty()) {
            cb(false, resp.error);
            return;
        }

        try {
            const auto root = nlohmann::json::parse(resp.body);
            const bool ok = root.value("code", -1) == 0;
            cb(ok, ok ? "" : root.value("message", "server error"));
        } catch (const std::exception& e) {
            cb(false, std::string("parse error: ") + e.what());
        }
    });
}

} // namespace anychat
