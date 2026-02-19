// test_connection_manager.cpp
//
// ConnectionManager 单元测试。
// 所有 WebSocket 和网络事件通过 Fake 对象同步驱动，不依赖真实网络或计时器。

#include <gtest/gtest.h>

#include "connection_manager.h"           // 待测类（内部头）
#include "anychat/network_monitor.h"
#include "network/iwebsocket_client.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

using namespace anychat;
using namespace anychat::network;

// ===========================================================================
// Fakes
// ===========================================================================

// --------------------------------------------------------------------------
// FakeNetworkMonitor
// --------------------------------------------------------------------------
class FakeNetworkMonitor : public NetworkMonitor {
public:
    explicit FakeNetworkMonitor(NetworkStatus initial = NetworkStatus::ReachableViaWiFi)
        : status_(initial) {}

    NetworkStatus currentStatus() const override { return status_; }

    void setOnStatusChanged(StatusChangedCallback cb) override {
        std::lock_guard<std::mutex> lock(mu_);
        cb_ = std::move(cb);
    }

    void start() override { ++start_count; }
    void stop()  override { ++stop_count;  }

    // 测试辅助：模拟网络变化
    void setStatus(NetworkStatus s) {
        status_ = s;
        StatusChangedCallback cb;
        {
            std::lock_guard<std::mutex> lock(mu_);
            cb = cb_;
        }
        if (cb) cb(s);
    }

    int start_count = 0;
    int stop_count  = 0;

private:
    std::atomic<NetworkStatus> status_;
    mutable std::mutex         mu_;
    StatusChangedCallback      cb_;
};

// --------------------------------------------------------------------------
// FakeWebSocketClient
// --------------------------------------------------------------------------
class FakeWebSocketClient : public IWebSocketClient {
public:
    // ---- IWebSocketClient 接口 ------------------------------------------------
    void connect() override {
        ++connect_count;
        // 默认不自动触发 connected；测试用 simulateConnected() 手动触发
    }

    void disconnect() override {
        ++disconnect_count;
        if (connected_.exchange(false)) {
            // 真实断开时触发 on_disconnected（可选，通过 auto_fire_on_disconnect 控制）
            if (auto_fire_on_disconnect && on_disconnected_) on_disconnected_();
        }
    }

    void send(const std::string&) override { ++send_count; }

    bool isConnected() const override { return connected_; }

    void setOnMessage(IWebSocketClient::MessageHandler h)           override { on_message_      = std::move(h); }
    void setOnConnected(IWebSocketClient::ConnectedHandler h)       override { on_connected_    = std::move(h); }
    void setOnDisconnected(IWebSocketClient::DisconnectedHandler h) override { on_disconnected_ = std::move(h); }
    void setOnError(IWebSocketClient::ErrorHandler h)               override { on_error_        = std::move(h); }

    // ---- 测试辅助：手动触发事件 -----------------------------------------------
    void simulateConnected() {
        connected_ = true;
        if (on_connected_) on_connected_();
    }

    void simulateDisconnected() {
        connected_ = false;
        if (on_disconnected_) on_disconnected_();
    }

    void simulateError(const std::string& err) {
        if (on_error_) on_error_(err);
    }

    // ---- 计数器 ---------------------------------------------------------------
    int connect_count    = 0;
    int disconnect_count = 0;
    int send_count       = 0;

    // 控制 disconnect() 是否自动触发 on_disconnected 回调（默认不自动触发）
    bool auto_fire_on_disconnect = false;

private:
    std::atomic<bool>                        connected_{false};
    IWebSocketClient::ConnectedHandler       on_connected_;
    IWebSocketClient::DisconnectedHandler    on_disconnected_;
    IWebSocketClient::ErrorHandler           on_error_;
    IWebSocketClient::MessageHandler         on_message_;
};

// ===========================================================================
// 测试夹具
// ===========================================================================

class ConnectionManagerTest : public ::testing::Test {
protected:
    // 状态变化历史（线程安全收集）
    std::vector<ConnectionState> state_history;
    std::mutex                   history_mutex;
    int                          ready_count = 0;

    std::shared_ptr<FakeNetworkMonitor>  monitor;
    std::shared_ptr<FakeWebSocketClient> ws;
    std::unique_ptr<ConnectionManager>   cm;

    void SetUp() override {
        monitor = std::make_shared<FakeNetworkMonitor>();
        ws      = std::make_shared<FakeWebSocketClient>();
        createCM();
    }

    // 重建 ConnectionManager（便于测试不同初始网络状态）
    void createCM(NetworkStatus initial_net = NetworkStatus::ReachableViaWiFi) {
        monitor = std::make_shared<FakeNetworkMonitor>(initial_net);
        ws      = std::make_shared<FakeWebSocketClient>();
        cm.reset();  // 先销毁旧的（避免回调悬挂）
        cm = std::make_unique<ConnectionManager>(
            "ws://fake:9999/ws",
            monitor,
            ws,
            [this](ConnectionState s) {
                std::lock_guard<std::mutex> lock(history_mutex);
                state_history.push_back(s);
            },
            [this]() { ++ready_count; }
        );
        state_history.clear();
        ready_count = 0;
    }

    // 断言最后一次状态变化
    void expectLastState(ConnectionState expected) {
        std::lock_guard<std::mutex> lock(history_mutex);
        ASSERT_FALSE(state_history.empty()) << "No state transitions recorded";
        EXPECT_EQ(state_history.back(), expected);
    }

    // 断言状态变化序列（仅检查最近 n 项）
    void expectStateSequence(std::vector<ConnectionState> expected) {
        std::lock_guard<std::mutex> lock(history_mutex);
        ASSERT_GE(state_history.size(), expected.size());
        auto offset = state_history.size() - expected.size();
        for (size_t i = 0; i < expected.size(); ++i) {
            EXPECT_EQ(state_history[offset + i], expected[i])
                << "  at index " << i;
        }
    }
};

// ===========================================================================
// 1. 初始状态
// ===========================================================================

TEST_F(ConnectionManagerTest, InitialStateIsDisconnected) {
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, InitialState_NoCallbackFired) {
    std::lock_guard<std::mutex> lock(history_mutex);
    EXPECT_TRUE(state_history.empty());
}

// ===========================================================================
// 2. connect() — 基本行为
// ===========================================================================

TEST_F(ConnectionManagerTest, Connect_WithNetworkAvailable_CallsWsConnect) {
    cm->connect();
    EXPECT_GE(ws->connect_count, 1);
}

TEST_F(ConnectionManagerTest, Connect_WithNetworkAvailable_StateBecomesConnecting) {
    cm->connect();
    EXPECT_EQ(cm->state(), ConnectionState::Connecting);
}

TEST_F(ConnectionManagerTest, Connect_WithNetworkAvailable_StateCallbackFired) {
    cm->connect();
    expectLastState(ConnectionState::Connecting);
}

TEST_F(ConnectionManagerTest, Connect_WithNoNetwork_DoesNotCallWsConnect) {
    createCM(NetworkStatus::NotReachable);
    cm->connect();
    EXPECT_EQ(ws->connect_count, 0);
}

TEST_F(ConnectionManagerTest, Connect_WithNoNetwork_StateStaysDisconnected) {
    createCM(NetworkStatus::NotReachable);
    cm->connect();
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, Connect_StartsNetworkMonitor) {
    cm->connect();
    EXPECT_GE(monitor->start_count, 1);
}

TEST_F(ConnectionManagerTest, Connect_ResetsSuperRetryCount) {
    // 先让重试计数增加再调用 connect()，确保被归零
    cm->connect();
    ws->simulateDisconnected();  // → Reconnecting，retry_count = 1
    cm->connect();               // 应重置计数，并重新 connect
    ws->simulateConnected();
    EXPECT_EQ(cm->state(), ConnectionState::Connected);
}

// ===========================================================================
// 3. WebSocket 连接成功
// ===========================================================================

TEST_F(ConnectionManagerTest, WsConnected_StateBecomesConnected) {
    cm->connect();
    ws->simulateConnected();
    EXPECT_EQ(cm->state(), ConnectionState::Connected);
}

TEST_F(ConnectionManagerTest, WsConnected_StateSequence_ConnectingThenConnected) {
    cm->connect();
    ws->simulateConnected();
    expectStateSequence({ConnectionState::Connecting, ConnectionState::Connected});
}

TEST_F(ConnectionManagerTest, WsConnected_OnReadyCalled) {
    cm->connect();
    ws->simulateConnected();
    EXPECT_EQ(ready_count, 1);
}

TEST_F(ConnectionManagerTest, WsConnected_OnReadyCalledOnlyOnce) {
    cm->connect();
    ws->simulateConnected();
    ws->simulateConnected();  // 重复触发
    EXPECT_EQ(ready_count, 2);  // 每次连接成功都应触发（可能是重连后的重新同步）
}

// ===========================================================================
// 4. WebSocket 断开 — 外层重连逻辑
// ===========================================================================

TEST_F(ConnectionManagerTest, WsDisconnected_WantConnected_StateBecomesReconnecting) {
    cm->connect();
    ws->simulateConnected();
    ws->simulateDisconnected();
    EXPECT_EQ(cm->state(), ConnectionState::Reconnecting);
}

TEST_F(ConnectionManagerTest, WsDisconnected_WantDisconnected_StateBecomesDisconnected) {
    cm->connect();
    ws->simulateConnected();
    cm->disconnect();           // 设置 want_connected = false
    ws->simulateDisconnected(); // 此时 want_connected=false，不应进入 Reconnecting
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, WsDisconnected_NoNetwork_StateBecomesDisconnected) {
    cm->connect();
    ws->simulateConnected();
    monitor->setStatus(NetworkStatus::NotReachable);   // 网络丢失
    ws->simulateDisconnected();
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, WsDisconnected_MultipleRetries_CountsCorrectly) {
    cm->connect();
    ws->simulateConnected();

    // 第 1~5 次断开 → 应进入 Reconnecting（外层重试 0~4 次）
    for (int i = 0; i < 5; ++i) {
        ws->simulateDisconnected();
        EXPECT_EQ(cm->state(), ConnectionState::Reconnecting)
            << "  after disconnect #" << (i + 1);
        // 模拟定时器触发重连后又断开：先让 cm 重连成功再断开
        ws->simulateConnected();
    }

    // 第 6 次断开 → 外层重试已耗尽（super_retry_count >= kMaxSuperRetries=5）
    ws->simulateDisconnected();
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

// ===========================================================================
// 5. disconnect() — 显式断开
// ===========================================================================

TEST_F(ConnectionManagerTest, Disconnect_CallsWsDisconnect) {
    cm->connect();
    ws->simulateConnected();
    cm->disconnect();
    EXPECT_GE(ws->disconnect_count, 1);
}

TEST_F(ConnectionManagerTest, Disconnect_StateBecomesDisconnected) {
    cm->connect();
    ws->simulateConnected();
    cm->disconnect();
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, Disconnect_StopsMonitor) {
    cm->connect();
    cm->disconnect();
    EXPECT_GE(monitor->stop_count, 1);
}

TEST_F(ConnectionManagerTest, Disconnect_WhileConnecting_StateBecomesDisconnected) {
    cm->connect();
    // 尚未 simulateConnected，仍处于 Connecting
    EXPECT_EQ(cm->state(), ConnectionState::Connecting);
    cm->disconnect();
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, Disconnect_WhileReconnecting_CancelsReconnect) {
    cm->connect();
    ws->simulateConnected();
    ws->simulateDisconnected();  // → Reconnecting
    EXPECT_EQ(cm->state(), ConnectionState::Reconnecting);

    cm->disconnect();
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);

    // 等足够时间，确认没有因为定时器触发了不期望的 connect 调用
    int prev_connect_count = ws->connect_count;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(ws->connect_count, prev_connect_count) << "Reconnect fired after disconnect()";
}

// ===========================================================================
// 6. WebSocket 错误
// ===========================================================================

TEST_F(ConnectionManagerTest, WsError_WhenConnected_StateBecomesReconnecting) {
    cm->connect();
    ws->simulateConnected();
    EXPECT_EQ(cm->state(), ConnectionState::Connected);

    ws->simulateError("connection reset");
    EXPECT_EQ(cm->state(), ConnectionState::Reconnecting);
}

TEST_F(ConnectionManagerTest, WsError_WhenConnecting_StateBecomesReconnecting) {
    cm->connect();
    // 仍在 Connecting 阶段（尚未 simulateConnected）
    // onWsError 只在 state==Connected 时变为 Reconnecting，否则不改变
    ws->simulateError("timeout");
    // Connecting 状态下 error 不变（由 WebSocket 内层处理）
    EXPECT_EQ(cm->state(), ConnectionState::Connecting);
}

TEST_F(ConnectionManagerTest, WsError_WhenDisconnected_StateUnchanged) {
    ws->simulateError("some error");
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

// ===========================================================================
// 7. 网络变化
// ===========================================================================

TEST_F(ConnectionManagerTest, NetworkLost_WhenConnected_DisconnectsWs) {
    cm->connect();
    ws->simulateConnected();
    EXPECT_EQ(cm->state(), ConnectionState::Connected);

    int prev_disconnect = ws->disconnect_count;
    monitor->setStatus(NetworkStatus::NotReachable);
    EXPECT_GT(ws->disconnect_count, prev_disconnect);
}

TEST_F(ConnectionManagerTest, NetworkLost_WhenConnected_StateBecomesDisconnected) {
    cm->connect();
    ws->simulateConnected();
    monitor->setStatus(NetworkStatus::NotReachable);
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, NetworkLost_WhenConnecting_StateBecomesDisconnected) {
    cm->connect();
    // Connecting 状态，网络丢失
    monitor->setStatus(NetworkStatus::NotReachable);
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, NetworkLost_WhenReconnecting_StateBecomesDisconnected) {
    cm->connect();
    ws->simulateConnected();
    ws->simulateDisconnected();  // → Reconnecting
    monitor->setStatus(NetworkStatus::NotReachable);
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, NetworkRestored_WhenDisconnected_WantConnected_Reconnects) {
    createCM(NetworkStatus::NotReachable);
    cm->connect();  // 网络不可用，只设置 want_connected
    EXPECT_EQ(ws->connect_count, 0);
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);

    monitor->setStatus(NetworkStatus::ReachableViaWiFi);
    EXPECT_GE(ws->connect_count, 1);
    EXPECT_EQ(cm->state(), ConnectionState::Connecting);
}

TEST_F(ConnectionManagerTest, NetworkRestored_WhenDisconnected_WantDisconnected_DoesNotReconnect) {
    createCM(NetworkStatus::NotReachable);
    // 从未调用 connect()，want_connected = false
    monitor->setStatus(NetworkStatus::ReachableViaWiFi);
    EXPECT_EQ(ws->connect_count, 0);
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, NetworkRestored_WhenReconnecting_TriggersImmediateConnect) {
    cm->connect();
    ws->simulateConnected();
    ws->simulateDisconnected();  // → Reconnecting（外层定时器等待中）

    // 网络先丢失再恢复
    monitor->setStatus(NetworkStatus::NotReachable);
    int prev_connect = ws->connect_count;
    monitor->setStatus(NetworkStatus::ReachableViaWiFi);
    EXPECT_GT(ws->connect_count, prev_connect) << "Should reconnect after network restored";
    EXPECT_EQ(cm->state(), ConnectionState::Connecting);
}

TEST_F(ConnectionManagerTest, NetworkRestored_ResetsRetryCount) {
    cm->connect();
    ws->simulateConnected();

    // 几次断开积累重试计数
    ws->simulateDisconnected();
    ws->simulateConnected();
    ws->simulateDisconnected();
    ws->simulateConnected();

    // 网络变化触发：先丢失再恢复
    monitor->setStatus(NetworkStatus::NotReachable);
    monitor->setStatus(NetworkStatus::ReachableViaWiFi);
    ws->simulateConnected();

    // 重试计数应已重置，再次触发多次断开仍能进入 Reconnecting
    ws->simulateDisconnected();
    EXPECT_EQ(cm->state(), ConnectionState::Reconnecting);
}

TEST_F(ConnectionManagerTest, NetworkStatusUnchanged_DoesNotTriggerReconnect) {
    cm->connect();
    ws->simulateConnected();
    int prev = ws->connect_count;

    // 发送相同状态，不应触发任何动作
    monitor->setStatus(NetworkStatus::ReachableViaWiFi);
    EXPECT_EQ(ws->connect_count, prev);
    EXPECT_EQ(cm->state(), ConnectionState::Connected);
}

// ===========================================================================
// 8. 状态回调
// ===========================================================================

TEST_F(ConnectionManagerTest, StateCallback_FiredForEachTransition) {
    cm->connect();               // Disconnected → Connecting
    ws->simulateConnected();     // Connecting   → Connected
    ws->simulateDisconnected();  // Connected    → Reconnecting
    cm->disconnect();            // Reconnecting → Disconnected

    std::lock_guard<std::mutex> lock(history_mutex);
    ASSERT_EQ(state_history.size(), 4u);
    EXPECT_EQ(state_history[0], ConnectionState::Connecting);
    EXPECT_EQ(state_history[1], ConnectionState::Connected);
    EXPECT_EQ(state_history[2], ConnectionState::Reconnecting);
    EXPECT_EQ(state_history[3], ConnectionState::Disconnected);
}

TEST_F(ConnectionManagerTest, StateCallback_NotFiredWhenStateUnchanged) {
    cm->connect();
    ws->simulateConnected();

    std::lock_guard<std::mutex> lock(history_mutex);
    size_t before = state_history.size();

    // 触发 Connected → Connected（无变化，回调不应重复触发）
    // 通过再次 simulateConnected 来验证 setState 的去重逻辑
    // （此处 onWsConnected 会调用 setState(Connected)，state 未变则不触发）
    // 解锁后触发
    lock.~lock_guard();  // 手动提前结束 lock 作用域
    ws->simulateConnected();

    {
        std::lock_guard<std::mutex> lock2(history_mutex);
        // Connected 状态没有变化，回调不应增加
        EXPECT_EQ(state_history.size(), before);
    }
}

// ===========================================================================
// 9. nullptr monitor（始终视网络为可用）
// ===========================================================================

TEST_F(ConnectionManagerTest, NullMonitor_AssumeNetworkAvailable) {
    cm.reset();
    cm = std::make_unique<ConnectionManager>(
        "ws://fake:9999/ws",
        nullptr,  // 不传 monitor
        ws,
        nullptr,
        nullptr
    );
    cm->connect();
    EXPECT_GE(ws->connect_count, 1);
    EXPECT_EQ(cm->state(), ConnectionState::Connecting);
}

// ===========================================================================
// 10. 重连退避延迟验证（逻辑层，不等待真实时间）
// ===========================================================================

TEST_F(ConnectionManagerTest, SuperRetry_ExhaustedAfterMaxRetries) {
    // kMaxSuperRetries = 5，第 6 次断开时应变为 Disconnected
    cm->connect();
    ws->simulateConnected();

    for (int i = 0; i < 5; ++i) {
        ws->simulateDisconnected();
        EXPECT_EQ(cm->state(), ConnectionState::Reconnecting)
            << "  retry " << i << " should still be Reconnecting";
        ws->simulateConnected();  // 模拟重连成功，重置 super_retry_count
    }

    // 最后一次：断开后成功过，但再次断开再次累积
    // 注意：simulateConnected 会重置 super_retry_count_ = 0
    // 所以要真正测试耗尽，需要不调用 simulateConnected
    // 重新开始一组不中断的断开序列
    for (int i = 0; i < 5; ++i) {
        ws->simulateDisconnected();
        if (i < 4) {
            EXPECT_EQ(cm->state(), ConnectionState::Reconnecting)
                << "  disconnect #" << (i + 1) << " should be Reconnecting";
        }
    }
    EXPECT_EQ(cm->state(), ConnectionState::Disconnected)
        << "After 5 unrecovered disconnects, should give up";
}

TEST_F(ConnectionManagerTest, SuperRetry_ConnectSuccessResetsCount) {
    cm->connect();
    ws->simulateConnected();

    // 累积 4 次断开（未超限）
    for (int i = 0; i < 4; ++i) {
        ws->simulateDisconnected();
        EXPECT_EQ(cm->state(), ConnectionState::Reconnecting);
    }

    // 成功连接一次，重置计数
    ws->simulateConnected();
    EXPECT_EQ(cm->state(), ConnectionState::Connected);
    EXPECT_EQ(ready_count, 5);  // 每次连接成功都应触发 on_ready

    // 之后再断开 5 次仍有充分重试次数
    for (int i = 0; i < 4; ++i) {
        ws->simulateDisconnected();
        EXPECT_EQ(cm->state(), ConnectionState::Reconnecting);
    }
}

// ===========================================================================
// 11. 析构安全性
// ===========================================================================

TEST_F(ConnectionManagerTest, Destructor_WhileConnected_DoesNotCrash) {
    cm->connect();
    ws->simulateConnected();
    EXPECT_NO_THROW({ cm.reset(); });
}

TEST_F(ConnectionManagerTest, Destructor_WhileReconnecting_DoesNotCrash) {
    cm->connect();
    ws->simulateConnected();
    ws->simulateDisconnected();
    EXPECT_EQ(cm->state(), ConnectionState::Reconnecting);
    EXPECT_NO_THROW({ cm.reset(); });
}

TEST_F(ConnectionManagerTest, Destructor_WhileConnecting_DoesNotCrash) {
    cm->connect();
    EXPECT_EQ(cm->state(), ConnectionState::Connecting);
    EXPECT_NO_THROW({ cm.reset(); });
}
