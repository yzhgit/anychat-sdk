#include "anychat/client.h"
#include "anychat/auth.h"
#include "anychat/message.h"
#include "anychat/types.h"

#include "auth_manager.h"
#include "connection_manager.h"
#include "db/database.h"
#include "cache/conversation_cache.h"
#include "cache/message_cache.h"
#include "notification_manager.h"
#include "outbound_queue.h"
#include "sync_engine.h"
#include "network/http_client.h"
#include "network/websocket_client.h"
#include "message_manager.h"
#include "conversation_manager.h"
#include "friend_manager.h"
#include "group_manager.h"
#include "file_manager.h"
#include "user_manager.h"
#include "rtc_manager.h"

#include <mutex>
#include <stdexcept>
#include <string>

namespace anychat {

// ---------------------------------------------------------------------------
// AnyChatClientImpl
// ---------------------------------------------------------------------------

class AnyChatClientImpl : public AnyChatClient {
public:
    explicit AnyChatClientImpl(const ClientConfig& config)
        : http_(std::make_shared<network::HttpClient>(config.api_base_url))
        , ws_(std::make_shared<network::WebSocketClient>(buildWsUrl(config)))
    {
        // 1. Open DB
        db_ = std::make_unique<db::Database>(config.db_path);
        if (!config.db_path.empty()) db_->open();

        // 2. Initialize caches
        conv_cache_ = std::make_unique<cache::ConversationCache>();
        msg_cache_  = std::make_unique<cache::MessageCache>();

        // 3. Create auth manager with DB for token persistence
        auth_mgr_ = createAuthManager(http_, config.device_id, db_.get());

        // 4. Create NotificationManager and wire WebSocket message handler
        notif_mgr_ = std::make_unique<NotificationManager>();
        ws_->setOnMessage([this](const std::string& raw) {
            notif_mgr_->handleRaw(raw);
        });

        // 5. Create OutboundQueue
        outbound_q_ = std::make_unique<OutboundQueue>(db_.get());

        // 6. Create SyncEngine
        sync_engine_ = std::make_unique<SyncEngine>(
            db_.get(), conv_cache_.get(), msg_cache_.get(), http_);

        // 7. Wire NotificationManager → OutboundQueue
        notif_mgr_->setOnMessageSent([this](const MsgSentAck& ack) {
            outbound_q_->onMessageSentAck(ack);
        });

        // Wire NotificationManager → ConnectionManager heartbeat (pong)
        notif_mgr_->setOnPong([this]() {
            conn_mgr_->onPongReceived();
        });

        // 8. Create Phase 4 business managers
        msg_mgr_ = std::make_unique<MessageManagerImpl>(
            db_.get(), msg_cache_.get(), outbound_q_.get(), notif_mgr_.get(), http_, "");
        conv_mgr_   = std::make_unique<ConversationManagerImpl>(
            db_.get(), conv_cache_.get(), notif_mgr_.get(), http_);
        friend_mgr_ = std::make_unique<FriendManagerImpl>(
            db_.get(), notif_mgr_.get(), http_);
        group_mgr_  = std::make_unique<GroupManagerImpl>(
            db_.get(), notif_mgr_.get(), http_);
        file_mgr_   = std::make_unique<FileManagerImpl>(http_);
        user_mgr_   = std::make_unique<UserManagerImpl>(http_);
        rtc_mgr_    = std::make_unique<RtcManagerImpl>(http_, notif_mgr_.get());

        // 9. Create ConnectionManager last (it calls onReady/onStateChanged)
        conn_mgr_ = std::make_unique<ConnectionManager>(
            buildWsUrl(config),
            config.network_monitor,
            ws_,
            [this](ConnectionState s) { onStateChanged(s); },
            [this]() { onReady(); });
    }

    ~AnyChatClientImpl() override {
        // Ensure clean shutdown: stop reconnects and close WebSocket.
        conn_mgr_->disconnect();
    }

    // ---- Lifecycle -----------------------------------------------------------

    void connect() override {
        conn_mgr_->connect();
    }

    void disconnect() override {
        outbound_q_->onDisconnected();
        conn_mgr_->disconnect();
    }

    ConnectionState connectionState() const override {
        return conn_mgr_->state();
    }

    void setOnConnectionStateChanged(ConnectionStateCallback callback) override {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        state_cb_ = std::move(callback);
    }

    // ---- Sub-modules ---------------------------------------------------------

    AuthManager& auth() override {
        return *auth_mgr_;
    }

    MessageManager& message() override {
        return *msg_mgr_;
    }

    ConversationManager& conversation() override {
        return *conv_mgr_;
    }

    FriendManager& friendMgr() override {
        return *friend_mgr_;
    }

    GroupManager& groupMgr() override {
        return *group_mgr_;
    }

    FileManager& fileMgr() override {
        return *file_mgr_;
    }

    UserManager& userMgr() override {
        return *user_mgr_;
    }

    RtcManager& rtcMgr() override {
        return *rtc_mgr_;
    }

private:
    // ---- Helpers -------------------------------------------------------------

    static std::string buildWsUrl(const ClientConfig& config) {
        // Append /api/v1/ws?token= — the token is filled in dynamically by
        // HttpClient / AuthManager; the WebSocket URL base is the gateway_url.
        std::string url = config.gateway_url;
        if (!url.empty() && url.back() != '/') url += '/';
        url += "api/v1/ws";
        return url;
    }

    void onStateChanged(ConnectionState s) {
        ConnectionStateCallback cb;
        {
            std::lock_guard<std::mutex> lock(cb_mutex_);
            cb = state_cb_;
        }
        if (cb) cb(s);
    }

    void onReady() {
        // WebSocket connected: flush outbound queue and trigger incremental sync.
        outbound_q_->onConnected([this](const std::string& json) {
            ws_->send(json);
        });
        sync_engine_->sync();
    }

    // ---- Members -------------------------------------------------------------

    std::shared_ptr<network::HttpClient>       http_;
    std::shared_ptr<network::WebSocketClient>  ws_;

    std::unique_ptr<db::Database>               db_;
    std::unique_ptr<cache::ConversationCache>   conv_cache_;
    std::unique_ptr<cache::MessageCache>        msg_cache_;

    std::unique_ptr<AuthManager>               auth_mgr_;
    std::unique_ptr<MessageManager>            msg_mgr_;
    std::unique_ptr<ConnectionManager>         conn_mgr_;

    std::unique_ptr<NotificationManager>        notif_mgr_;
    std::unique_ptr<OutboundQueue>              outbound_q_;
    std::unique_ptr<SyncEngine>                 sync_engine_;

    std::unique_ptr<ConversationManager>        conv_mgr_;
    std::unique_ptr<FriendManager>              friend_mgr_;
    std::unique_ptr<GroupManager>               group_mgr_;
    std::unique_ptr<FileManager>                file_mgr_;
    std::unique_ptr<UserManager>                user_mgr_;
    std::unique_ptr<RtcManager>                 rtc_mgr_;

    mutable std::mutex      cb_mutex_;
    ConnectionStateCallback state_cb_;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::shared_ptr<AnyChatClient> AnyChatClient::create(const ClientConfig& config) {
    if (config.gateway_url.empty())
        throw std::invalid_argument("ClientConfig::gateway_url must not be empty");
    if (config.api_base_url.empty())
        throw std::invalid_argument("ClientConfig::api_base_url must not be empty");
    if (config.device_id.empty())
        throw std::invalid_argument("ClientConfig::device_id must not be empty");

    return std::make_shared<AnyChatClientImpl>(config);
}

} // namespace anychat
