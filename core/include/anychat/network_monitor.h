#pragma once

#include <functional>

namespace anychat {

// Network reachability status
enum class NetworkStatus
{
    Unknown, // Initial state, not yet detected
    NotReachable, // No network
    ReachableViaWiFi, // WiFi connection
    ReachableViaCellular, // Cellular connection
};

inline bool isReachable(NetworkStatus s) {
    return s == NetworkStatus::ReachableViaWiFi || s == NetworkStatus::ReachableViaCellular;
}

// Abstract interface, implemented by each platform binding and injected into ClientConfig.
//
// Platform implementation references:
//   Android  : ConnectivityManager + NetworkCallback
//   iOS/macOS: NWPathMonitor (iOS 12+) or SCNetworkReachability
//   Linux    : netlink socket monitoring RTM_NEWROUTE / RTM_DELROUTE
//   Web      : navigator.onLine + 'online'/'offline' events
//
// Thread requirements:
//   - currentStatus() must be thread-safe.
//   - Callbacks registered via setOnStatusChanged() may be invoked on any platform thread,
//     SDK will lock internally when invoking callbacks, no additional synchronization needed from platform side.
class NetworkMonitor {
public:
    using StatusChangedCallback = std::function<void(NetworkStatus)>;

    virtual ~NetworkMonitor() = default;

    // Returns current network status (returns immediately, does not block).
    virtual NetworkStatus currentStatus() const = 0;

    // Register status change callback. Call before start().
    virtual void setOnStatusChanged(StatusChangedCallback cb) = 0;

    // Start listening for network changes.
    virtual void start() = 0;

    // Stop listening, release system resources.
    virtual void stop() = 0;
};

} // namespace anychat
