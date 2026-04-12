#pragma once

#include "auth.h"
#include "conversation.h"
#include "file.h"
#include "friend.h"
#include "group.h"
#include "message.h"
#include "network_monitor.h"
#include "call.h"
#include "types.h"
#include "user.h"
#include "version.h"

#include <functional>
#include <memory>
#include <string>

namespace anychat {

struct ClientConfig {
    // ---- Network ------------------------------------------------------------
    std::string gateway_url; // WebSocket gateway, e.g. "wss://api.anychat.io"
    std::string api_base_url; // HTTP API base path, e.g. "https://api.anychat.io/api/v1"

    // ---- Device -------------------------------------------------------------
    std::string device_id; // Unique device identifier, generated and persisted by platform binding
        // Android: Settings.Secure.ANDROID_ID or UUID
        // iOS: UIDevice.identifierForVendor
        // Web: UUID stored in localStorage

    // ---- Persistence --------------------------------------------------------
    std::string db_path; // SQLite database file full path, injected by platform binding
        // Android: Context.getDatabasePath("anychat.db").absolutePath
        // iOS: <ApplicationSupport>/anychat.db
        // Web: leave empty (Web SDK uses IndexedDB, not through C++ Core)

    // ---- Network Monitor -----------------------------------------------------
    // Optional. Platform-implemented NetworkMonitor; when nullptr, SDK always considers network available.
    std::shared_ptr<NetworkMonitor> network_monitor;

    // ---- Connection Parameters -----------------------------------------------
    int connect_timeout_ms = 10'000;
    int max_reconnect_attempts = 5; // WebSocket inner layer max retry attempts
    bool auto_reconnect = true; // Whether to auto-reconnect after disconnection
};

using ConnectionStateCallback = std::function<void(ConnectionState state)>;

class AnyChatClient {
public:
    static std::shared_ptr<AnyChatClient> create(const ClientConfig& config);

    virtual ~AnyChatClient() = default;

    // ---- Auth & Connection Management ---------------------------------
    // login(): HTTP auth + auto-establish WebSocket connection
    // logout(): disconnect WebSocket + HTTP logout
    // Note: WebSocket auto-reconnect is managed internally by SDK, no manual connect call needed
    // client_version: client version string (e.g. "1.0.0")
    virtual void login(
        const std::string& account,
        const std::string& password,
        const std::string& device_type,
        const std::string& client_version,
        AuthCallback callback
    ) = 0;

    virtual void logout(ResultCallback callback) = 0;

    virtual bool isLoggedIn() const = 0;
    virtual AuthToken getCurrentToken() const = 0;

    virtual ConnectionState connectionState() const = 0;
    virtual void setOnConnectionStateChanged(ConnectionStateCallback callback) = 0;

    // ---- Sub-modules -------------------------------------------------------
    virtual AuthManager& authMgr() = 0;
    virtual MessageManager& messageMgr() = 0;
    virtual ConversationManager& conversationMgr() = 0;
    virtual FriendManager& friendMgr() = 0;
    virtual GroupManager& groupMgr() = 0;
    virtual FileManager& fileMgr() = 0;
    virtual UserManager& userMgr() = 0;
    virtual CallManager& callMgr() = 0;
    virtual VersionManager& versionMgr() = 0;
};

} // namespace anychat
