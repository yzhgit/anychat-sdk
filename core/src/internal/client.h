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
#include <mutex>
#include <string>

namespace anychat {

namespace cache {
class ConversationCache;
class MessageCache;
} // namespace cache

namespace db {
class Database;
} // namespace db

namespace network {
class HttpClient;
class WebSocketClient;
} // namespace network

class ConnectionManager;
class NotificationManager;
class OutboundQueue;
class SyncEngine;

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
    explicit AnyChatClient(const ClientConfig& config);
    ~AnyChatClient();

    // ---- Auth & Connection Management ---------------------------------
    // login(): HTTP auth + auto-establish WebSocket connection
    // logout(): disconnect WebSocket + HTTP logout
    // Note: WebSocket auto-reconnect is managed internally by SDK, no manual connect call needed
    // client_version: client version string (e.g. "1.0.0")
    void login(
        const std::string& account,
        const std::string& password,
        const std::string& device_type,
        const std::string& client_version,
        AnyChatValueCallback<AuthToken> callback
    );

    void logout(AnyChatCallback callback);

    bool isLoggedIn() const;
    AuthToken getCurrentToken() const;

    ConnectionState connectionState() const;
    void setOnConnectionStateChanged(ConnectionStateCallback callback);

    // ---- Sub-modules -------------------------------------------------------
    AuthManager& authMgr();
    MessageManager& messageMgr();
    ConversationManager& conversationMgr();
    FriendManager& friendMgr();
    GroupManager& groupMgr();
    FileManager& fileMgr();
    UserManager& userMgr();
    CallManager& callMgr();
    VersionManager& versionMgr();

private:
    static std::string buildWsUrl(const std::string& gateway_url, const std::string& token);
    void initializeWebSocket(const std::string& access_token);
    void onStateChanged(ConnectionState state);
    void onReady();

    std::shared_ptr<network::HttpClient> http_;
    std::shared_ptr<network::WebSocketClient> ws_;

    std::string gateway_url_;
    ClientConfig config_;

    std::unique_ptr<db::Database> db_;
    std::unique_ptr<cache::ConversationCache> conv_cache_;
    std::unique_ptr<cache::MessageCache> msg_cache_;

    std::unique_ptr<AuthManager> auth_mgr_;
    std::unique_ptr<MessageManager> msg_mgr_;
    std::unique_ptr<ConnectionManager> conn_mgr_;

    std::unique_ptr<NotificationManager> notif_mgr_;
    std::unique_ptr<OutboundQueue> outbound_q_;
    std::unique_ptr<SyncEngine> sync_engine_;

    std::unique_ptr<ConversationManager> conv_mgr_;
    std::unique_ptr<FriendManager> friend_mgr_;
    std::unique_ptr<GroupManager> group_mgr_;
    std::unique_ptr<FileManager> file_mgr_;
    std::unique_ptr<UserManager> user_mgr_;
    std::unique_ptr<CallManager> call_mgr_;
    std::unique_ptr<VersionManager> version_mgr_;

    mutable std::mutex cb_mutex_;
    ConnectionStateCallback state_cb_;
};

} // namespace anychat
