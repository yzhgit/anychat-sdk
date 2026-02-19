#include "websocket_client.h"

#include <libwebsockets.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <queue>
#include <thread>

namespace anychat::network {

// ── constants ─────────────────────────────────────────────────────────────────

static constexpr int  HEARTBEAT_INTERVAL_S = 30;
static constexpr int  MAX_RECONNECT        = 5;
static constexpr long RECONNECT_BASE_MS    = 1000; // base for 2^n back-off

// ── Impl ──────────────────────────────────────────────────────────────────────

struct WebSocketClient::Impl {
    // ── config ────────────────────────────────────────────────────────────────
    std::string url;
    std::string host;
    std::string path;
    int         port      = 443;
    bool        use_ssl   = true;

    // ── lws objects ──────────────────────────────────────────────────────────
    lws_context* ctx = nullptr;
    lws*         wsi = nullptr;

    // ── state ─────────────────────────────────────────────────────────────────
    std::atomic<bool>  connected{false};
    std::atomic<bool>  running{false};
    int                reconnect_count = 0;
    std::chrono::steady_clock::time_point last_ping;

    // ── outbound queue ────────────────────────────────────────────────────────
    std::mutex         send_mutex;
    std::queue<std::string> send_queue;

    // ── worker thread ─────────────────────────────────────────────────────────
    std::thread worker;

    // ── user callbacks ────────────────────────────────────────────────────────
    std::mutex                        cb_mutex;
    WebSocketClient::MessageHandler      on_message;
    WebSocketClient::ConnectedHandler    on_connected;
    WebSocketClient::DisconnectedHandler on_disconnected;
    WebSocketClient::ErrorHandler        on_error;

    // ── parse url ─────────────────────────────────────────────────────────────
    void parse_url() {
        // Supports ws:// and wss://
        use_ssl = (url.substr(0, 6) == "wss://");
        size_t scheme_end = url.find("://") + 3;
        size_t path_start = url.find('/', scheme_end);
        if (path_start == std::string::npos) {
            host = url.substr(scheme_end);
            path = "/";
        } else {
            host = url.substr(scheme_end, path_start - scheme_end);
            path = url.substr(path_start);
        }
        size_t colon = host.rfind(':');
        if (colon != std::string::npos) {
            port = std::stoi(host.substr(colon + 1));
            host = host.substr(0, colon);
        } else {
            port = use_ssl ? 443 : 80;
        }
    }

    // ── lws callback (static, dispatches to instance) ─────────────────────────
    static int lws_callback(lws* wsi_in, lws_callback_reasons reason,
                            void* user, void* in, size_t len) {
        auto* self = static_cast<Impl*>(lws_context_user(lws_get_context(wsi_in)));
        if (!self) return 0;

        switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED: {
            self->connected      = true;
            self->reconnect_count = 0;
            self->last_ping = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lk(self->cb_mutex);
            if (self->on_connected) self->on_connected();
            lws_callback_on_writable(wsi_in);
            break;
        }
        case LWS_CALLBACK_CLIENT_RECEIVE: {
            std::string msg(static_cast<const char*>(in), len);
            std::lock_guard<std::mutex> lk(self->cb_mutex);
            if (self->on_message) self->on_message(msg);
            break;
        }
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            // Send ping if interval elapsed
            auto now     = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                               now - self->last_ping).count();
            if (elapsed >= HEARTBEAT_INTERVAL_S) {
                const std::string ping = R"({"type":"ping"})";
                std::vector<unsigned char> buf(LWS_PRE + ping.size());
                std::memcpy(buf.data() + LWS_PRE, ping.data(), ping.size());
                lws_write(wsi_in, buf.data() + LWS_PRE,
                          ping.size(), LWS_WRITE_TEXT);
                self->last_ping = now;
            }
            // Drain outbound queue
            std::lock_guard<std::mutex> lk(self->send_mutex);
            if (!self->send_queue.empty()) {
                const std::string& msg = self->send_queue.front();
                std::vector<unsigned char> buf(LWS_PRE + msg.size());
                std::memcpy(buf.data() + LWS_PRE, msg.data(), msg.size());
                lws_write(wsi_in, buf.data() + LWS_PRE,
                          msg.size(), LWS_WRITE_TEXT);
                self->send_queue.pop();
                if (!self->send_queue.empty())
                    lws_callback_on_writable(wsi_in);
            }
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLOSED: {
            self->connected = false;
            self->wsi       = nullptr;
            {
                std::lock_guard<std::mutex> lk(self->cb_mutex);
                if (self->on_disconnected) self->on_disconnected();
                if (reason == LWS_CALLBACK_CLIENT_CONNECTION_ERROR) {
                    const char* err = in ? static_cast<const char*>(in) : "unknown";
                    if (self->on_error) self->on_error(err);
                }
            }
            break;
        }
        default:
            break;
        }
        return 0;
    }

    // ── event loop ─────────────────────────────────────────────────────────────
    void loop() {
        static const lws_protocols protocols[] = {
            {"anychat", lws_callback, 0, 4096, 0, nullptr, 0},
            {nullptr,   nullptr,      0, 0,    0, nullptr, 0}
        };

        lws_context_creation_info info{};
        info.port      = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;
        info.user      = this;
        if (use_ssl)
            info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

        ctx = lws_create_context(&info);
        if (!ctx) {
            std::lock_guard<std::mutex> lk(cb_mutex);
            if (on_error) on_error("lws_create_context failed");
            return;
        }

        do_connect();

        while (running) {
            lws_service(ctx, 20 /*ms*/);

            // Schedule writable callback to flush queue / send ping
            if (connected && wsi)
                lws_callback_on_writable(wsi);

            // Reconnect with exponential back-off
            if (!connected && running && reconnect_count < MAX_RECONNECT) {
                long wait_ms = RECONNECT_BASE_MS * (1L << reconnect_count);
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(wait_ms));
                reconnect_count++;
                do_connect();
            }
        }

        lws_context_destroy(ctx);
        ctx = nullptr;
    }

    void do_connect() {
        lws_client_connect_info ci{};
        ci.context        = ctx;
        ci.address        = host.c_str();
        ci.port           = port;
        ci.path           = path.c_str();
        ci.host           = host.c_str();
        ci.origin         = host.c_str();
        ci.protocol       = "anychat";
        ci.ssl_connection = use_ssl ? LCCSCF_USE_SSL : 0;
        wsi = lws_client_connect_via_info(&ci);
    }
};

// ── WebSocketClient public API ────────────────────────────────────────────────

WebSocketClient::WebSocketClient(std::string url)
    : impl_(std::make_unique<Impl>()) {
    impl_->url = std::move(url);
    impl_->parse_url();
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

void WebSocketClient::connect() {
    if (impl_->running) return;
    impl_->running = true;
    impl_->worker  = std::thread(&Impl::loop, impl_.get());
}

void WebSocketClient::disconnect() {
    impl_->running = false;
    if (impl_->worker.joinable()) impl_->worker.join();
}

void WebSocketClient::send(const std::string& message) {
    {
        std::lock_guard<std::mutex> lk(impl_->send_mutex);
        impl_->send_queue.push(message);
    }
    if (impl_->connected && impl_->wsi)
        lws_callback_on_writable(impl_->wsi);
}

bool WebSocketClient::isConnected() const {
    return impl_->connected;
}

void WebSocketClient::setOnMessage(MessageHandler handler) {
    std::lock_guard<std::mutex> lk(impl_->cb_mutex);
    impl_->on_message = std::move(handler);
}

void WebSocketClient::setOnConnected(ConnectedHandler handler) {
    std::lock_guard<std::mutex> lk(impl_->cb_mutex);
    impl_->on_connected = std::move(handler);
}

void WebSocketClient::setOnDisconnected(DisconnectedHandler handler) {
    std::lock_guard<std::mutex> lk(impl_->cb_mutex);
    impl_->on_disconnected = std::move(handler);
}

void WebSocketClient::setOnError(ErrorHandler handler) {
    std::lock_guard<std::mutex> lk(impl_->cb_mutex);
    impl_->on_error = std::move(handler);
}

} // namespace anychat::network
