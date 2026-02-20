#pragma once

#include "types.h"
#include "auth.h"
#include "message.h"
#include "conversation.h"
#include "friend.h"
#include "group.h"
#include "file.h"
#include "user.h"
#include "rtc.h"
#include "network_monitor.h"
#include <memory>
#include <string>
#include <functional>

namespace anychat {

struct ClientConfig {
    // ---- 网络 ----------------------------------------------------------------
    std::string gateway_url;     // WebSocket 网关，e.g. "wss://api.anychat.io"
    std::string api_base_url;    // HTTP API 根路径，e.g. "https://api.anychat.io/api/v1"

    // ---- 设备 ----------------------------------------------------------------
    std::string device_id;       // 唯一设备标识，由平台 binding 生成并持久化
                                 // Android: Settings.Secure.ANDROID_ID 或 UUID
                                 // iOS: UIDevice.identifierForVendor
                                 // Web: localStorage 中存储的 UUID

    // ---- 持久化 --------------------------------------------------------------
    std::string db_path;         // SQLite 数据库文件完整路径，由平台 binding 注入
                                 // Android: Context.getDatabasePath("anychat.db").absolutePath
                                 // iOS: <ApplicationSupport>/anychat.db
                                 // Web: 留空（Web SDK 使用 IndexedDB，不经过 C++ Core）

    // ---- 网络监控 ------------------------------------------------------------
    // 可选。注入平台实现的 NetworkMonitor；为 nullptr 时 SDK 始终视网络为可用。
    std::shared_ptr<NetworkMonitor> network_monitor;

    // ---- 连接参数 ------------------------------------------------------------
    int  connect_timeout_ms      = 10'000;
    int  max_reconnect_attempts  = 5;      // WebSocket 内层最大重试次数
    bool auto_reconnect          = true;   // 断线后是否自动重连
};

using ConnectionStateCallback = std::function<void(ConnectionState state)>;

class AnyChatClient {
public:
    static std::shared_ptr<AnyChatClient> create(const ClientConfig& config);

    virtual ~AnyChatClient() = default;

    // ---- 生命周期 ------------------------------------------------------------
    // connect()    : 表达"我希望保持连接"的意图，内部处理重连。
    // disconnect() : 显式断开，停止重连。
    virtual void connect()    = 0;
    virtual void disconnect() = 0;

    virtual ConnectionState connectionState() const = 0;
    virtual void setOnConnectionStateChanged(ConnectionStateCallback callback) = 0;

    // ---- 子模块 --------------------------------------------------------------
    virtual AuthManager&         authMgr()         = 0;
    virtual MessageManager&      messageMgr()      = 0;
    virtual ConversationManager& conversationMgr() = 0;
    virtual FriendManager&       friendMgr()    = 0;
    virtual GroupManager&        groupMgr()     = 0;
    virtual FileManager&         fileMgr()      = 0;
    virtual UserManager&         userMgr()      = 0;
    virtual RtcManager&          rtcMgr()       = 0;
};

} // namespace anychat
