#pragma once

#include "auth_manager.h"
#include "call_manager.h"
#include "conversation_manager.h"
#include "file_manager.h"
#include "friend_manager.h"
#include "group_manager.h"
#include "message_manager.h"
#include "network_monitor.h"
#include "sdk_callbacks.h"
#include "sdk_types.h"
#include "user_manager.h"
#include "version_manager.h"

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
        int32_t device_type,
        const std::string& client_version,
        AnyChatValueCallback<AuthToken> callback
    );

    void logout(AnyChatCallback callback);

    bool isLoggedIn() const;
    AuthToken getCurrentToken() const;

    ConnectionState connectionState() const;
    void setOnConnectionStateChanged(ConnectionStateCallback callback);

    // ---- Sub-modules -------------------------------------------------------
    AuthManagerImpl& authMgr();
    MessageManagerImpl& messageMgr();
    ConversationManagerImpl& conversationMgr();
    FriendManagerImpl& friendMgr();
    GroupManagerImpl& groupMgr();
    FileManagerImpl& fileMgr();
    UserManagerImpl& userMgr();
    CallManagerImpl& callMgr();
    VersionManagerImpl& versionMgr();

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

    std::unique_ptr<AuthManagerImpl> auth_mgr_;
    std::unique_ptr<MessageManagerImpl> msg_mgr_;
    std::unique_ptr<ConnectionManager> conn_mgr_;

    std::unique_ptr<NotificationManager> notif_mgr_;
    std::unique_ptr<OutboundQueue> outbound_q_;
    std::unique_ptr<SyncEngine> sync_engine_;

    std::unique_ptr<ConversationManagerImpl> conv_mgr_;
    std::unique_ptr<FriendManagerImpl> friend_mgr_;
    std::unique_ptr<GroupManagerImpl> group_mgr_;
    std::unique_ptr<FileManagerImpl> file_mgr_;
    std::unique_ptr<UserManagerImpl> user_mgr_;
    std::unique_ptr<CallManagerImpl> call_mgr_;
    std::unique_ptr<VersionManagerImpl> version_mgr_;

    mutable std::mutex cb_mutex_;
    ConnectionStateCallback state_cb_;
};

} // namespace anychat
