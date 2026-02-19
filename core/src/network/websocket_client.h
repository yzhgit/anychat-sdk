#pragma once

#include "iwebsocket_client.h"
#include <memory>
#include <string>

namespace anychat::network {

// Async WebSocket client backed by libwebsockets.
// The internal event loop runs on a dedicated thread.
// All handlers are invoked from that thread — callers must synchronise if needed.
class WebSocketClient : public IWebSocketClient {
public:
    // |url| must be a full ws:// or wss:// URI,
    // e.g. "wss://api.example.com/api/v1/ws?token=..."
    explicit WebSocketClient(std::string url);
    ~WebSocketClient() override;

    WebSocketClient(const WebSocketClient&)            = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;

    void connect()    override;
    void disconnect() override;
    void send(const std::string& message) override;
    bool isConnected() const override;

    void setOnMessage(MessageHandler handler)           override;
    void setOnConnected(ConnectedHandler handler)       override;
    void setOnDisconnected(DisconnectedHandler handler) override;
    void setOnError(ErrorHandler handler)               override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace anychat::network
