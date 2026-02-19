#pragma once

#include "anychat/types.h"
#include "anychat/network_monitor.h"
#include "network/iwebsocket_client.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace anychat {

// ConnectionManager 是 SDK 网络连接的中枢状态机。
//
// 职责：
//   1. 将 NetworkMonitor 的网络可达性变化映射为 WebSocket 的暂停/恢复。
//   2. 维护对外可见的 ConnectionState，并在状态变化时触发回调。
//   3. 在 WebSocket 的内部重试耗尽后，执行更长间隔的外层重连（最多 kMaxSuperRetries 次）。
//   4. WebSocket 连接建立后触发 on_ready 钩子（供 SyncEngine 执行增量同步）。
//   5. 在连接期间每 30 s 发送一次 ping 心跳，若 60 s 内未收到 pong 则触发重连。
//
// 与 WebSocketClient 的分工：
//   WebSocketClient  — 处理连接级重试（快速，最多 5 次，指数退避至 ~16 s）
//   ConnectionManager — 处理网络级重试（慢速，最多 5 次，退避从 30 s 开始）
//                       以及网络可达性引起的暂停 / 恢复
class ConnectionManager {
public:
    // |ws_url|    : 完整 WebSocket 地址，token 由 HttpClient 维护，以 query 形式附加。
    // |monitor|   : 平台注入的网络监视器，可为 nullptr（表示始终认为网络可用）。
    // |ws|        : 已构造的 WebSocket 客户端。
    // |on_state_changed| : ConnectionState 变化时通知调用方。
    // |on_ready|  : WebSocket 成功建立后调用（用于触发增量同步等后连接动作）。
    ConnectionManager(std::string ws_url,
                      std::shared_ptr<NetworkMonitor> monitor,
                      std::shared_ptr<network::IWebSocketClient> ws,
                      std::function<void(ConnectionState)> on_state_changed,
                      std::function<void()> on_ready);

    ~ConnectionManager();

    ConnectionManager(const ConnectionManager&)            = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // 表达"我希望保持连接"的意图。
    // 若网络当前可用，立即开始连接；否则等待网络恢复后自动连接。
    void connect();

    // 表达"我希望断开连接"的意图。
    // 取消所有待重连的计划，关闭 WebSocket，停止网络监听。
    void disconnect();

    ConnectionState state() const;

    // Called by the notification layer (e.g. NotificationManager) when a
    // "pong" frame is received from the server.  Updates last_pong_ms_ so the
    // heartbeat watchdog knows the connection is still alive.
    void onPongReceived();

private:
    // ---- 内部事件处理 ----------------------------------------------------------

    void onNetworkChanged(NetworkStatus status);
    void onWsConnected();
    void onWsDisconnected();
    void onWsError(const std::string& error);

    // ---- 动作 -----------------------------------------------------------------

    // 真正触发 WebSocket 连接（只在 want_connected_ && network_ok_ 时调用）。
    void doConnect();

    // 关闭 WebSocket 并取消所有重连计划。
    void doDisconnect();

    // 调度一次外层重连（延迟 delay_ms 毫秒后调用 doConnect）。
    void scheduleReconnect(int delay_ms);

    // 唤醒重连线程，取消待定的重连计划。
    void cancelReconnect();

    // 线程安全地更新 state_ 并触发回调。
    void setState(ConnectionState s);

    // ---- 心跳 -----------------------------------------------------------------

    // Start the heartbeat loop thread.  Called from onWsConnected().
    void startHeartbeat();

    // Signal the heartbeat thread to stop and join it.  Called from
    // onWsDisconnected() and doDisconnect().
    void stopHeartbeat();

    // Send a ping frame to the server.
    void sendPing();

    // ---- 重连参数 --------------------------------------------------------------

    // 外层重连（ConnectionManager 级别）基础延迟 30 s，最多 5 次。
    static constexpr int kSuperBaseDelayMs  = 30'000;
    static constexpr int kMaxSuperRetries   = 5;

    // Heartbeat parameters.
    static constexpr int kHeartbeatIntervalMs = 30'000;  // send ping every 30 s
    static constexpr int kPongTimeoutMs       = 60'000;  // 2 missed pongs = 60 s

    // ---- 成员 -----------------------------------------------------------------

    std::string                                  ws_url_;
    std::shared_ptr<NetworkMonitor>              monitor_;
    std::shared_ptr<network::IWebSocketClient>    ws_;
    std::function<void(ConnectionState)>         on_state_changed_;
    std::function<void()>                        on_ready_;

    std::atomic<ConnectionState>  state_{ConnectionState::Disconnected};
    std::atomic<bool>             want_connected_{false};  // 用户意图
    std::atomic<bool>             network_ok_{true};       // 当前网络是否可达
    std::atomic<int>              super_retry_count_{0};   // 外层重连计数

    // 回调保护
    std::mutex cb_mutex_;

    // 外层重连定时器（独立线程 + condition_variable 实现可取消的 sleep）
    std::thread              reconnect_thread_;
    std::mutex               reconnect_mutex_;
    std::condition_variable  reconnect_cv_;
    bool                     reconnect_pending_  = false;
    bool                     reconnect_cancel_   = false;
    bool                     stopping_           = false;
    int                      reconnect_delay_ms_ = 0;

    // ---- 心跳线程成员 ----------------------------------------------------------

    std::thread              heartbeat_thread_;
    std::mutex               heartbeat_mutex_;
    std::condition_variable  heartbeat_cv_;
    bool                     heartbeat_stopping_ = false;

    // Unix timestamp (ms) of the last received pong.  Initialised to "now"
    // when the heartbeat starts so the first 30 s window is clean.
    std::atomic<int64_t>     last_pong_ms_{0};
};

} // namespace anychat
