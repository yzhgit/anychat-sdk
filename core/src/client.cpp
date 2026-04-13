#include "anychat/client.h"

#include "auth_manager.h"
#include "call_manager.h"
#include "connection_manager.h"
#include "conversation_manager.h"
#include "file_manager.h"
#include "friend_manager.h"
#include "group_manager.h"
#include "message_manager.h"
#include "notification_manager.h"
#include "outbound_queue.h"
#include "sync_engine.h"
#include "user_manager.h"
#include "version_manager.h"

#include "anychat/auth.h"
#include "anychat/message.h"
#include "anychat/types.h"

#include "cache/conversation_cache.h"
#include "cache/message_cache.h"
#include "db/database.h"
#include "network/http_client.h"
#include "network/websocket_client.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace anychat {

class AnyChatClientImpl : public AnyChatClient {
public:
    explicit AnyChatClientImpl(const ClientConfig& config)
        : http_(std::make_shared<network::HttpClient>(config.api_base_url))
        , gateway_url_(config.gateway_url)
        , config_(config) {
        db_ = std::make_unique<db::Database>(config.db_path);
        if (!config.db_path.empty()) {
            db_->open();
        }

        conv_cache_ = std::make_unique<cache::ConversationCache>();
        msg_cache_ = std::make_unique<cache::MessageCache>();
        notif_mgr_ = std::make_unique<NotificationManager>();

        auth_mgr_ = createAuthManager(http_, config.device_id, db_.get(), notif_mgr_.get());
        outbound_q_ = std::make_unique<OutboundQueue>(db_.get());
        sync_engine_ = std::make_unique<SyncEngine>(db_.get(), conv_cache_.get(), msg_cache_.get(), http_);

        notif_mgr_->setOnMessageSent([this](const MsgSentAck& ack) {
            outbound_q_->onMessageSentAck(ack);
        });

        msg_mgr_ = std::make_unique<MessageManagerImpl>(
            db_.get(),
            msg_cache_.get(),
            outbound_q_.get(),
            notif_mgr_.get(),
            http_,
            ""
        );
        conv_mgr_ = std::make_unique<ConversationManagerImpl>(db_.get(), conv_cache_.get(), notif_mgr_.get(), http_);
        friend_mgr_ = std::make_unique<FriendManagerImpl>(db_.get(), notif_mgr_.get(), http_);
        group_mgr_ = std::make_unique<GroupManagerImpl>(db_.get(), notif_mgr_.get(), http_);
        file_mgr_ = std::make_unique<FileManagerImpl>(http_);
        user_mgr_ = std::make_unique<UserManagerImpl>(http_, notif_mgr_.get(), config.device_id);
        call_mgr_ = std::make_unique<CallManagerImpl>(http_, notif_mgr_.get());
        version_mgr_ = std::make_unique<VersionManagerImpl>(http_);
    }

    ~AnyChatClientImpl() override {
        if (conn_mgr_) {
            conn_mgr_->disconnect();
        }
    }

    void login(
        const std::string& account,
        const std::string& password,
        const std::string& device_type,
        const std::string& client_version,
        AnyChatValueCallback<AuthToken> callback
    ) override {
        auto cb_ptr = std::make_shared<AnyChatValueCallback<AuthToken>>(std::move(callback));
        auth_mgr_->login(
            account,
            password,
            device_type,
            client_version,
            AnyChatValueCallback<AuthToken>{
                .on_success =
                    [this, cb_ptr](const AuthToken& token) {
                        initializeWebSocket(token.access_token);
                        conn_mgr_->connect();
                        if (cb_ptr->on_success) {
                            cb_ptr->on_success(token);
                        }
                    },
                .on_error =
                    [cb_ptr](int code, const std::string& error) {
                        if (cb_ptr->on_error) {
                            cb_ptr->on_error(code, error);
                        }
                    },
            }
        );
    }

    void logout(AnyChatCallback callback) override {
        if (conn_mgr_) {
            conn_mgr_->disconnect();
            outbound_q_->onDisconnected();
        }

        auth_mgr_->logout(std::move(callback));
    }

    bool isLoggedIn() const override {
        return auth_mgr_->isLoggedIn();
    }

    AuthToken getCurrentToken() const override {
        return auth_mgr_->currentToken();
    }

    ConnectionState connectionState() const override {
        if (!conn_mgr_) {
            return ConnectionState::Disconnected;
        }
        return conn_mgr_->state();
    }

    void setOnConnectionStateChanged(ConnectionStateCallback callback) override {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        state_cb_ = std::move(callback);
    }

    AuthManager& authMgr() override {
        return *auth_mgr_;
    }

    MessageManager& messageMgr() override {
        return *msg_mgr_;
    }

    ConversationManager& conversationMgr() override {
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

    CallManager& callMgr() override {
        return *call_mgr_;
    }

    VersionManager& versionMgr() override {
        return *version_mgr_;
    }

private:
    static std::string buildWsUrl(const std::string& gateway_url, const std::string& token) {
        std::string url = gateway_url;
        if (!url.empty() && url.back() != '/') {
            url += '/';
        }
        url += "api/v1/ws";
        if (!token.empty()) {
            url += "?token=" + token;
        }
        return url;
    }

    void initializeWebSocket(const std::string& access_token) {
        const std::string ws_url = buildWsUrl(gateway_url_, access_token);

        ws_ = std::make_shared<network::WebSocketClient>(ws_url);
        ws_->setOnMessage([this](const std::string& raw) {
            notif_mgr_->handleRaw(raw);
        });

        conn_mgr_ = std::make_unique<ConnectionManager>(
            ws_url,
            config_.network_monitor,
            ws_,
            [this](ConnectionState state) {
                onStateChanged(state);
            },
            [this]() {
                onReady();
            }
        );

        notif_mgr_->setOnPong([this]() {
            if (conn_mgr_) {
                conn_mgr_->onPongReceived();
            }
        });
    }

    void onStateChanged(ConnectionState state) {
        ConnectionStateCallback callback;
        {
            std::lock_guard<std::mutex> lock(cb_mutex_);
            callback = state_cb_;
        }
        if (callback) {
            callback(state);
        }
    }

    void onReady() {
        if (ws_) {
            outbound_q_->onConnected([this](const std::string& json) {
                ws_->send(json);
            });
        }
        sync_engine_->sync();
    }

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

std::shared_ptr<AnyChatClient> AnyChatClient::create(const ClientConfig& config) {
    if (config.gateway_url.empty()) {
        throw std::invalid_argument("ClientConfig::gateway_url must not be empty");
    }
    if (config.api_base_url.empty()) {
        throw std::invalid_argument("ClientConfig::api_base_url must not be empty");
    }
    if (config.device_id.empty()) {
        throw std::invalid_argument("ClientConfig::device_id must not be empty");
    }

    return std::make_shared<AnyChatClientImpl>(config);
}

} // namespace anychat
