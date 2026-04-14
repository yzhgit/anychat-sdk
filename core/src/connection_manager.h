#pragma once

#include "internal/network_monitor.h"
#include "internal/types.h"

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

// ConnectionManager is the central state machine for SDK network connections.
//
// Responsibilities:
//   1. Map NetworkMonitor network reachability changes to WebSocket pause/resume.
//   2. Maintain externally visible ConnectionState and trigger callbacks on state changes.
//   3. Perform longer-interval outer-layer reconnection after WebSocket internal retries exhausted (up to kMaxSuperRetries times).
//   4. Trigger on_ready hook after WebSocket connection established (for SyncEngine to perform incremental sync).
//   5. Send ping heartbeat every 30 s during connection; reconnect if no pong received within 60 s.
//
// Division with WebSocketClient:
//   WebSocketClient  — handles connection-level retries (fast, up to 5 times, exponential backoff to ~16 s)
//   ConnectionManager — handles network-level retries (slow, up to 5 times, backoff starts at 30 s)
//                       and network reachability-induced pause/resume
class ConnectionManager {
public:
    // |ws_url|    : full WebSocket URL, token maintained by HttpClient, appended as query.
    // |monitor|   : platform-injected network monitor, can be nullptr (means always consider network available).
    // |ws|        : constructed WebSocket client.
    // |on_state_changed| : notifies caller when ConnectionState changes.
    // |on_ready|  : called after WebSocket successfully established (for post-connect actions like incremental sync).
    ConnectionManager(
        std::string ws_url,
        std::shared_ptr<NetworkMonitor> monitor,
        std::shared_ptr<network::IWebSocketClient> ws,
        std::function<void(ConnectionState)> on_state_changed,
        std::function<void()> on_ready
    );

    ~ConnectionManager();

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // Express intent to "keep connected".
    // If network is currently available, connect immediately; otherwise wait for network recovery then auto-connect.
    void connect();

    // Express intent to "disconnect".
    // Cancel all pending reconnection plans, close WebSocket, stop network monitoring.
    void disconnect();

    ConnectionState state() const;

    // Called by the notification layer (e.g. NotificationManager) when a
    // "pong" frame is received from the server.  Updates last_pong_ms_ so the
    // heartbeat watchdog knows the connection is still alive.
    void onPongReceived();

private:
    // ---- Internal Event Handling --------------------------------------------

    void onNetworkChanged(NetworkStatus status);
    void onWsConnected();
    void onWsDisconnected();
    void onWsError(const std::string& error);

    // ---- Actions ------------------------------------------------------------

    // Actually trigger WebSocket connection (only called when want_connected_ && network_ok_).
    void doConnect();

    // Close WebSocket and cancel all reconnection plans.
    void doDisconnect();

    // Schedule an outer-layer reconnection (calls doConnect after delay_ms milliseconds).
    void scheduleReconnect(int delay_ms);

    // Wake up reconnect thread, cancel pending reconnection plans.
    void cancelReconnect();

    // Thread-safe update of state_ and trigger callback.
    void setState(ConnectionState s);

    // ---- Heartbeat -----------------------------------------------------------

    // Start the heartbeat loop thread.  Called from onWsConnected().
    void startHeartbeat();

    // Signal the heartbeat thread to stop and join it.  Called from
    // onWsDisconnected() and doDisconnect().
    void stopHeartbeat();

    // Send a ping frame to the server.
    void sendPing();

    // ---- Reconnect Parameters -----------------------------------------------

    // Outer-layer reconnection (ConnectionManager level) base delay 30 s, max 5 times.
    static constexpr int kSuperBaseDelayMs = 30'000;
    static constexpr int kMaxSuperRetries = 5;

    // Heartbeat parameters.
    static constexpr int kHeartbeatIntervalMs = 30'000; // send ping every 30 s
    static constexpr int kPongTimeoutMs = 60'000; // 2 missed pongs = 60 s

    // ---- Members --------------------------------------------------------------

    std::string ws_url_;
    std::shared_ptr<NetworkMonitor> monitor_;
    std::shared_ptr<network::IWebSocketClient> ws_;
    std::function<void(ConnectionState)> on_state_changed_;
    std::function<void()> on_ready_;

    std::atomic<ConnectionState> state_{ ConnectionState::Disconnected };
    std::atomic<bool> want_connected_{ false }; // user intent
    std::atomic<bool> network_ok_{ true }; // current network reachability
    std::atomic<int> super_retry_count_{ 0 }; // outer-layer retry count

    // callback protection
    std::mutex cb_mutex_;

    // outer-layer reconnect timer (separate thread + condition_variable for cancellable sleep)
    std::thread reconnect_thread_;
    std::mutex reconnect_mutex_;
    std::condition_variable reconnect_cv_;
    bool reconnect_pending_ = false;
    bool reconnect_cancel_ = false;
    bool stopping_ = false;
    int reconnect_delay_ms_ = 0;

    // ---- Heartbeat Thread Members ----------------------------------------

    std::thread heartbeat_thread_;
    std::mutex heartbeat_mutex_;
    std::condition_variable heartbeat_cv_;
    bool heartbeat_stopping_ = false;

    // Unix timestamp (ms) of the last received pong.  Initialised to "now"
    // when the heartbeat starts so the first 30 s window is clean.
    std::atomic<int64_t> last_pong_ms_{ 0 };
};

} // namespace anychat
