#include "connection_manager.h"

#include <algorithm>
#include <chrono>

namespace anychat {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

int64_t currentMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

ConnectionManager::ConnectionManager(
    std::string ws_url,
    std::shared_ptr<NetworkMonitor> monitor,
    std::shared_ptr<network::IWebSocketClient> ws,
    std::function<void(ConnectionState)> on_state_changed,
    std::function<void()> on_ready
)
    : ws_url_(std::move(ws_url))
    , monitor_(std::move(monitor))
    , ws_(std::move(ws))
    , on_state_changed_(std::move(on_state_changed))
    , on_ready_(std::move(on_ready)) {
    // Subscribe to WebSocket events
    ws_->setOnConnected([this]() {
        onWsConnected();
    });
    ws_->setOnDisconnected([this]() {
        onWsDisconnected();
    });
    ws_->setOnError([this](const std::string& e) {
        onWsError(e);
    });

    // Initialize network status
    if (monitor_) {
        network_ok_ = isReachable(monitor_->currentStatus());
        monitor_->setOnStatusChanged([this](NetworkStatus s) {
            onNetworkChanged(s);
        });
    }

    // Start reconnect timer thread (resident, waiting to be woken)
    reconnect_thread_ = std::thread([this]() {
        while (true) {
            std::unique_lock<std::mutex> lock(reconnect_mutex_);
            // Wait until there's pending reconnect or stop signal
            reconnect_cv_.wait(lock, [this]() {
                return reconnect_pending_ || stopping_;
            });
            if (stopping_)
                break;

            // Extract delay time, unlock then sleep
            reconnect_cancel_ = false;
            lock.unlock();

            // Use condition_variable for interruptible wait
            // Delay time already set in reconnect_delay_ms_ by scheduleReconnect
            {
                std::unique_lock<std::mutex> delay_lock(reconnect_mutex_);
                reconnect_cv_.wait_for(delay_lock, std::chrono::milliseconds(reconnect_delay_ms_), [this]() {
                    return reconnect_cancel_ || stopping_;
                });

                reconnect_pending_ = false;
                if (reconnect_cancel_ || stopping_)
                    continue;
            }

            // Timer expired and not cancelled — execute connection
            if (want_connected_ && network_ok_) {
                doConnect();
            }
        }
    });
}

ConnectionManager::~ConnectionManager() {
    // Stop the heartbeat thread before joining the reconnect thread so we
    // don't call onWsDisconnected() after teardown.
    stopHeartbeat();

    {
        std::lock_guard<std::mutex> lock(reconnect_mutex_);
        stopping_ = true;
        reconnect_cancel_ = true;
        reconnect_pending_ = true; // wake up thread
    }
    reconnect_cv_.notify_all();
    if (reconnect_thread_.joinable())
        reconnect_thread_.join();

    if (monitor_)
        monitor_->stop();
    ws_->disconnect();
}

// ---------------------------------------------------------------------------
// Public Interface
// ---------------------------------------------------------------------------

void ConnectionManager::connect() {
    want_connected_ = true;
    super_retry_count_ = 0;
    cancelReconnect();

    if (monitor_)
        monitor_->start();

    if (network_ok_) {
        doConnect();
    } else {
        // Network unavailable, wait for onNetworkChanged to trigger
        setState(ConnectionState::Disconnected);
    }
}

void ConnectionManager::disconnect() {
    want_connected_ = false;
    cancelReconnect();
    stopHeartbeat();
    doDisconnect();
    if (monitor_)
        monitor_->stop();
}

ConnectionState ConnectionManager::state() const {
    return state_.load();
}

void ConnectionManager::onPongReceived() {
    last_pong_ms_.store(currentMs(), std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Internal Event Handling
// ---------------------------------------------------------------------------

void ConnectionManager::onNetworkChanged(NetworkStatus status) {
    bool reachable = isReachable(status);
    bool was_ok = network_ok_.exchange(reachable);

    if (reachable == was_ok)
        return; // State unchanged, ignore

    if (reachable) {
        // Network recovered — if user intent is to connect, reconnect immediately
        if (want_connected_ && (state_ == ConnectionState::Disconnected || state_ == ConnectionState::Reconnecting)) {
            super_retry_count_ = 0;
            cancelReconnect();
            doConnect();
        }
    } else {
        // Network lost — pause all reconnection, close existing connection
        cancelReconnect();
        if (state_ != ConnectionState::Disconnected) {
            stopHeartbeat();
            doDisconnect();
            // Keep want_connected_ as true, for auto-reconnect when network recovers
        }
    }
}

void ConnectionManager::onWsConnected() {
    super_retry_count_ = 0;
    setState(ConnectionState::Connected);

    // Start heartbeat now that the connection is live.
    startHeartbeat();

    // Trigger post-connect hook (incremental sync, etc.)
    std::function<void()> cb;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        cb = on_ready_;
    }
    if (cb)
        cb();
}

void ConnectionManager::onWsDisconnected() {
    // Stop heartbeat — the connection is gone.
    stopHeartbeat();

    // WebSocket internal retries exhausted, hand over to ConnectionManager for outer-layer reconnection
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
    // Outer-layer backoff: 30 s × 2^n, cap at 5 minutes
    int delay = kSuperBaseDelayMs * (1 << std::min(retries, 4));
    scheduleReconnect(delay);
}

void ConnectionManager::onWsError(const std::string& /*error*/) {
    // WebSocket layer already retried internally, just update upper-layer state here
    if (state_ == ConnectionState::Connected)
        setState(ConnectionState::Reconnecting);
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------

void ConnectionManager::doConnect() {
    setState(ConnectionState::Connecting);
    ws_->connect();
}

void ConnectionManager::doDisconnect() {
    ws_->disconnect();
    setState(ConnectionState::Disconnected);
}

void ConnectionManager::scheduleReconnect(int delay_ms) {
    std::lock_guard<std::mutex> lock(reconnect_mutex_);
    reconnect_delay_ms_ = delay_ms;
    reconnect_cancel_ = false;
    reconnect_pending_ = true;
    reconnect_cv_.notify_one();
}

void ConnectionManager::cancelReconnect() {
    std::lock_guard<std::mutex> lock(reconnect_mutex_);
    reconnect_cancel_ = true;
    reconnect_pending_ = false;
    reconnect_cv_.notify_one();
}

void ConnectionManager::setState(ConnectionState s) {
    ConnectionState prev = state_.exchange(s);
    if (prev == s)
        return;

    std::function<void(ConnectionState)> cb;
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        cb = on_state_changed_;
    }
    if (cb)
        cb(s);
}

// ---------------------------------------------------------------------------
// Heartbeat
// ---------------------------------------------------------------------------

void ConnectionManager::startHeartbeat() {
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
                heartbeat_cv_.wait_for(lock, std::chrono::milliseconds(kHeartbeatIntervalMs), [this]() {
                    return heartbeat_stopping_;
                });

                if (heartbeat_stopping_)
                    break;
            }

            // Only send pings (and check for pong timeout) while connected.
            if (state_.load() != ConnectionState::Connected)
                continue;

            // Check whether the server has been silent for too long.
            int64_t elapsed = currentMs() - last_pong_ms_.load(std::memory_order_relaxed);
            if (elapsed > kPongTimeoutMs) {
                // Two consecutive pongs missed — treat as a dead connection.
                onWsDisconnected();
                break; // stopHeartbeat() will be called by onWsDisconnected
            }

            // Send the ping frame.
            sendPing();
        }
    });
}

void ConnectionManager::stopHeartbeat() {
    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex_);
        heartbeat_stopping_ = true;
    }
    heartbeat_cv_.notify_all();

    if (heartbeat_thread_.joinable())
        heartbeat_thread_.join();
}

void ConnectionManager::sendPing() {
    // Best-effort: ignore errors (the pong timeout will catch dead connections).
    try {
        ws_->send("{\"type\":\"ping\"}");
    } catch (...) {
        // Swallow — connection may have already closed.
    }
}

} // namespace anychat
