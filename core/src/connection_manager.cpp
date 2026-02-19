#include "connection_manager.h"

#include <algorithm>
#include <chrono>

namespace anychat {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

int64_t currentMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(
               steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------

ConnectionManager::ConnectionManager(
    std::string ws_url,
    std::shared_ptr<NetworkMonitor> monitor,
    std::shared_ptr<network::IWebSocketClient> ws,
    std::function<void(ConnectionState)> on_state_changed,
    std::function<void()> on_ready)
    : ws_url_(std::move(ws_url))
    , monitor_(std::move(monitor))
    , ws_(std::move(ws))
    , on_state_changed_(std::move(on_state_changed))
    , on_ready_(std::move(on_ready))
{
    // 订阅 WebSocket 事件
    ws_->setOnConnected([this]() { onWsConnected(); });
    ws_->setOnDisconnected([this]() { onWsDisconnected(); });
    ws_->setOnError([this](const std::string& e) { onWsError(e); });

    // 初始化网络状态
    if (monitor_) {
        network_ok_ = isReachable(monitor_->currentStatus());
        monitor_->setOnStatusChanged([this](NetworkStatus s) {
            onNetworkChanged(s);
        });
    }

    // 启动重连定时器线程（常驻，等待被唤醒）
    reconnect_thread_ = std::thread([this]() {
        while (true) {
            std::unique_lock<std::mutex> lock(reconnect_mutex_);
            // 等待直到有待处理的重连或停止信号
            reconnect_cv_.wait(lock, [this]() {
                return reconnect_pending_ || stopping_;
            });
            if (stopping_) break;

            // 取出延迟时间，解锁后再 sleep
            reconnect_cancel_ = false;
            lock.unlock();

            // 采用 condition_variable 实现可中断的等待
            // 延迟时间已由 scheduleReconnect 设置在 reconnect_delay_ms_
            {
                std::unique_lock<std::mutex> delay_lock(reconnect_mutex_);
                reconnect_cv_.wait_for(delay_lock,
                    std::chrono::milliseconds(reconnect_delay_ms_),
                    [this]() { return reconnect_cancel_ || stopping_; });

                reconnect_pending_ = false;
                if (reconnect_cancel_ || stopping_) continue;
            }

            // 定时器到期，且未被取消 — 执行连接
            if (want_connected_ && network_ok_) {
                doConnect();
            }
        }
    });
}

ConnectionManager::~ConnectionManager()
{
    // Stop the heartbeat thread before joining the reconnect thread so we
    // don't call onWsDisconnected() after teardown.
    stopHeartbeat();

    {
        std::lock_guard<std::mutex> lock(reconnect_mutex_);
        stopping_          = true;
        reconnect_cancel_  = true;
        reconnect_pending_ = true;  // 唤醒线程
    }
    reconnect_cv_.notify_all();
    if (reconnect_thread_.joinable())
        reconnect_thread_.join();

    if (monitor_) monitor_->stop();
    ws_->disconnect();
}

// ---------------------------------------------------------------------------
// 公开接口
// ---------------------------------------------------------------------------

void ConnectionManager::connect()
{
    want_connected_ = true;
    super_retry_count_ = 0;
    cancelReconnect();

    if (monitor_) monitor_->start();

    if (network_ok_) {
        doConnect();
    } else {
        // 网络不可用，等待 onNetworkChanged 触发
        setState(ConnectionState::Disconnected);
    }
}

void ConnectionManager::disconnect()
{
    want_connected_ = false;
    cancelReconnect();
    stopHeartbeat();
    doDisconnect();
    if (monitor_) monitor_->stop();
}

ConnectionState ConnectionManager::state() const
{
    return state_.load();
}

void ConnectionManager::onPongReceived()
{
    last_pong_ms_.store(currentMs(), std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// 内部事件处理
// ---------------------------------------------------------------------------

void ConnectionManager::onNetworkChanged(NetworkStatus status)
{
    bool reachable = isReachable(status);
    bool was_ok    = network_ok_.exchange(reachable);

    if (reachable == was_ok) return;  // 状态未变，忽略

    if (reachable) {
        // 网络恢复 — 如果用户意图是连接，立刻重连
        if (want_connected_ &&
            (state_ == ConnectionState::Disconnected ||
             state_ == ConnectionState::Reconnecting))
        {
            super_retry_count_ = 0;
            cancelReconnect();
            doConnect();
        }
    } else {
        // 网络丢失 — 暂停一切重连，关闭现有连接
        cancelReconnect();
        if (state_ != ConnectionState::Disconnected) {
            stopHeartbeat();
            doDisconnect();
            // 保留 want_connected_ 为 true，等网络恢复后自动重连
        }
    }
}

void ConnectionManager::onWsConnected()
{
    super_retry_count_ = 0;
    setState(ConnectionState::Connected);

    // Start heartbeat now that the connection is live.
    startHeartbeat();

    // 触发后连接钩子（增量同步等）
    std::function<void()> cb;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        cb = on_ready_;
    }
    if (cb) cb();
}

void ConnectionManager::onWsDisconnected()
{
    // Stop heartbeat — the connection is gone.
    stopHeartbeat();

    // WebSocket 内部重试已耗尽，交由 ConnectionManager 做外层重连
    if (!want_connected_ || !network_ok_) {
        setState(ConnectionState::Disconnected);
        return;
    }

    int retries = super_retry_count_.fetch_add(1);
    if (retries >= kMaxSuperRetries) {
        setState(ConnectionState::Disconnected);
        return;
    }

    setState(ConnectionState::Reconnecting);
    // 外层退避：30 s × 2^n，上限 5 分钟
    int delay = kSuperBaseDelayMs * (1 << std::min(retries, 4));
    scheduleReconnect(delay);
}

void ConnectionManager::onWsError(const std::string& /*error*/)
{
    // WebSocket 层已经在内部重试，此处仅更新上层状态
    if (state_ == ConnectionState::Connected)
        setState(ConnectionState::Reconnecting);
}

// ---------------------------------------------------------------------------
// 动作
// ---------------------------------------------------------------------------

void ConnectionManager::doConnect()
{
    setState(ConnectionState::Connecting);
    ws_->connect();
}

void ConnectionManager::doDisconnect()
{
    ws_->disconnect();
    setState(ConnectionState::Disconnected);
}

void ConnectionManager::scheduleReconnect(int delay_ms)
{
    std::lock_guard<std::mutex> lock(reconnect_mutex_);
    reconnect_delay_ms_ = delay_ms;
    reconnect_cancel_   = false;
    reconnect_pending_  = true;
    reconnect_cv_.notify_one();
}

void ConnectionManager::cancelReconnect()
{
    std::lock_guard<std::mutex> lock(reconnect_mutex_);
    reconnect_cancel_  = true;
    reconnect_pending_ = false;
    reconnect_cv_.notify_one();
}

void ConnectionManager::setState(ConnectionState s)
{
    ConnectionState prev = state_.exchange(s);
    if (prev == s) return;

    std::function<void(ConnectionState)> cb;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        cb = on_state_changed_;
    }
    if (cb) cb(s);
}

// ---------------------------------------------------------------------------
// 心跳
// ---------------------------------------------------------------------------

void ConnectionManager::startHeartbeat()
{
    // Guard against double-start: stop any existing thread first.
    stopHeartbeat();

    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex_);
        heartbeat_stopping_ = false;
    }

    // Initialise last_pong_ms_ to "now" so the very first interval is clean.
    last_pong_ms_.store(currentMs(), std::memory_order_relaxed);

    heartbeat_thread_ = std::thread([this]() {
        while (true) {
            // Sleep for the ping interval, but wake early if stopHeartbeat()
            // is called.
            {
                std::unique_lock<std::mutex> lock(heartbeat_mutex_);
                heartbeat_cv_.wait_for(
                    lock,
                    std::chrono::milliseconds(kHeartbeatIntervalMs),
                    [this]() { return heartbeat_stopping_; });

                if (heartbeat_stopping_) break;
            }

            // Only send pings (and check for pong timeout) while connected.
            if (state_.load() != ConnectionState::Connected) continue;

            // Check whether the server has been silent for too long.
            int64_t elapsed = currentMs() - last_pong_ms_.load(std::memory_order_relaxed);
            if (elapsed > kPongTimeoutMs) {
                // Two consecutive pongs missed — treat as a dead connection.
                onWsDisconnected();
                break;  // stopHeartbeat() will be called by onWsDisconnected
            }

            // Send the ping frame.
            sendPing();
        }
    });
}

void ConnectionManager::stopHeartbeat()
{
    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex_);
        heartbeat_stopping_ = true;
    }
    heartbeat_cv_.notify_all();

    if (heartbeat_thread_.joinable())
        heartbeat_thread_.join();
}

void ConnectionManager::sendPing()
{
    // Best-effort: ignore errors (the pong timeout will catch dead connections).
    try {
        ws_->send("{\"type\":\"ping\"}");
    } catch (...) {
        // Swallow — connection may have already closed.
    }
}

} // namespace anychat
