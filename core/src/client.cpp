#include "internal/client.h"

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

#include "internal/auth.h"
#include "internal/message.h"
#include "internal/types.h"

#include "cache/conversation_cache.h"
#include "cache/message_cache.h"
#include "db/database.h"
#include "network/http_client.h"
#include "network/websocket_client.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace anychat {

AnyChatClient::AnyChatClient(const ClientConfig& config)
    : http_(std::make_shared<network::HttpClient>(config.api_base_url))
    , gateway_url_(config.gateway_url)
    , config_(config) {
    if (config.gateway_url.empty()) {
        throw std::invalid_argument("ClientConfig::gateway_url must not be empty");
    }
    if (config.api_base_url.empty()) {
        throw std::invalid_argument("ClientConfig::api_base_url must not be empty");
    }
    if (config.device_id.empty()) {
        throw std::invalid_argument("ClientConfig::device_id must not be empty");
    }

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

AnyChatClient::~AnyChatClient() {
    if (conn_mgr_) {
        conn_mgr_->disconnect();
    }
}

void AnyChatClient::login(
    const std::string& account,
    const std::string& password,
    const std::string& device_type,
    const std::string& client_version,
    AnyChatValueCallback<AuthToken> callback
) {
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

void AnyChatClient::logout(AnyChatCallback callback) {
    if (conn_mgr_) {
        conn_mgr_->disconnect();
        outbound_q_->onDisconnected();
    }

    auth_mgr_->logout(std::move(callback));
}

bool AnyChatClient::isLoggedIn() const {
    return auth_mgr_->isLoggedIn();
}

AuthToken AnyChatClient::getCurrentToken() const {
    return auth_mgr_->currentToken();
}

ConnectionState AnyChatClient::connectionState() const {
    if (!conn_mgr_) {
        return ConnectionState::Disconnected;
    }
    return conn_mgr_->state();
}

void AnyChatClient::setOnConnectionStateChanged(ConnectionStateCallback callback) {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    state_cb_ = std::move(callback);
}

AuthManager& AnyChatClient::authMgr() {
    return *auth_mgr_;
}

MessageManager& AnyChatClient::messageMgr() {
    return *msg_mgr_;
}

ConversationManager& AnyChatClient::conversationMgr() {
    return *conv_mgr_;
}

FriendManager& AnyChatClient::friendMgr() {
    return *friend_mgr_;
}

GroupManager& AnyChatClient::groupMgr() {
    return *group_mgr_;
}

FileManager& AnyChatClient::fileMgr() {
    return *file_mgr_;
}

UserManager& AnyChatClient::userMgr() {
    return *user_mgr_;
}

CallManager& AnyChatClient::callMgr() {
    return *call_mgr_;
}

VersionManager& AnyChatClient::versionMgr() {
    return *version_mgr_;
}

std::string AnyChatClient::buildWsUrl(const std::string& gateway_url, const std::string& token) {
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

void AnyChatClient::initializeWebSocket(const std::string& access_token) {
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

void AnyChatClient::onStateChanged(ConnectionState state) {
    ConnectionStateCallback callback;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        callback = state_cb_;
    }
    if (callback) {
        callback(state);
    }
}

void AnyChatClient::onReady() {
    if (ws_) {
        outbound_q_->onConnected([this](const std::string& json) {
            ws_->send(json);
        });
    }
    sync_engine_->sync();
}

} // namespace anychat
