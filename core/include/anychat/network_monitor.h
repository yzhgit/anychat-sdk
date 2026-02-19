#pragma once

#include <functional>

namespace anychat {

// 网络可达性状态
enum class NetworkStatus {
    Unknown,              // 初始态，尚未检测
    NotReachable,         // 无网络
    ReachableViaWiFi,     // WiFi 连接
    ReachableViaCellular, // 蜂窝网络连接
};

inline bool isReachable(NetworkStatus s) {
    return s == NetworkStatus::ReachableViaWiFi ||
           s == NetworkStatus::ReachableViaCellular;
}

// 抽象接口，由各平台 binding 实现后注入 ClientConfig。
//
// 平台实现参考：
//   Android  : ConnectivityManager + NetworkCallback
//   iOS/macOS: NWPathMonitor (iOS 12+) 或 SCNetworkReachability
//   Linux    : netlink socket 监听 RTM_NEWROUTE / RTM_DELROUTE
//   Web      : navigator.onLine + 'online'/'offline' 事件
//
// 线程要求：
//   - currentStatus() 必须线程安全。
//   - setOnStatusChanged() 注册的回调可在任意平台线程中被调用，
//     SDK 内部会在回调时加锁，无需平台侧额外同步。
class NetworkMonitor {
public:
    using StatusChangedCallback = std::function<void(NetworkStatus)>;

    virtual ~NetworkMonitor() = default;

    // 返回当前网络状态（立即返回，不阻塞）。
    virtual NetworkStatus currentStatus() const = 0;

    // 注册状态变化回调。在 start() 前调用。
    virtual void setOnStatusChanged(StatusChangedCallback cb) = 0;

    // 开始监听网络变化。
    virtual void start() = 0;

    // 停止监听，释放系统资源。
    virtual void stop() = 0;
};

} // namespace anychat
