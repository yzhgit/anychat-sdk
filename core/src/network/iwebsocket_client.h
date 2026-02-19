#pragma once

#include <functional>
#include <string>

namespace anychat::network {

// IWebSocketClient — WebSocketClient 的可测试抽象接口。
// 生产代码使用 WebSocketClient（libwebsockets），
// 测试代码使用 FakeWebSocketClient。
class IWebSocketClient {
public:
    using MessageHandler      = std::function<void(const std::string& msg)>;
    using ConnectedHandler    = std::function<void()>;
    using DisconnectedHandler = std::function<void()>;
    using ErrorHandler        = std::function<void(const std::string& error)>;

    virtual ~IWebSocketClient() = default;

    virtual void connect()    = 0;
    virtual void disconnect() = 0;
    virtual void send(const std::string& message) = 0;
    virtual bool isConnected() const = 0;

    virtual void setOnMessage(MessageHandler handler)         = 0;
    virtual void setOnConnected(ConnectedHandler handler)     = 0;
    virtual void setOnDisconnected(DisconnectedHandler handler) = 0;
    virtual void setOnError(ErrorHandler handler)             = 0;
};

} // namespace anychat::network
