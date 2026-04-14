/**
 * embind_wrapper.cpp - Emscripten embind bindings for AnyChat C API
 *
 * This file uses Emscripten's embind to wrap the C API into JavaScript-friendly
 * interfaces. The TypeScript layer (AnyChatClient.ts) provides a Promise-based
 * API on top of these bindings.
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <cstring>

#include "anychat/anychat.h"

using namespace emscripten;

std::string dispatchErrorMessage(int code) {
    return std::string("Request dispatch failed (code=") + std::to_string(code) + ")";
}

// ============================================================================
// Helper: Convert C structs to JavaScript objects
// ============================================================================

val authTokenToJS(const AnyChatAuthToken_C& token) {
    val obj = val::object();
    obj.set("accessToken", std::string(token.access_token));
    obj.set("refreshToken", std::string(token.refresh_token));
    obj.set("expiresAt", token.expires_at_ms);
    return obj;
}

val userInfoToJS(const AnyChatUserInfo_C& user) {
    val obj = val::object();
    obj.set("userId", std::string(user.user_id));
    obj.set("username", std::string(user.username));
    obj.set("avatarUrl", std::string(user.avatar_url));
    return obj;
}

val messageToJS(const AnyChatMessage_C& msg) {
    val obj = val::object();
    obj.set("messageId", std::string(msg.message_id));
    obj.set("localId", std::string(msg.local_id));
    obj.set("convId", std::string(msg.conv_id));
    obj.set("senderId", std::string(msg.sender_id));
    obj.set("contentType", std::string(msg.content_type));
    obj.set("type", msg.type);
    obj.set("content", msg.content ? std::string(msg.content) : "");
    obj.set("seq", (double)msg.seq);
    obj.set("replyTo", std::string(msg.reply_to));
    obj.set("timestamp", (double)msg.timestamp_ms);
    obj.set("status", msg.status);
    obj.set("sendState", msg.send_state);
    obj.set("isRead", msg.is_read != 0);
    return obj;
}

val conversationToJS(const AnyChatConversation_C& conv) {
    val obj = val::object();
    obj.set("convId", std::string(conv.conv_id));
    obj.set("convType", conv.conv_type);
    obj.set("targetId", std::string(conv.target_id));
    obj.set("lastMsgId", std::string(conv.last_msg_id));
    obj.set("lastMsgText", std::string(conv.last_msg_text));
    obj.set("lastMsgTime", (double)conv.last_msg_time_ms);
    obj.set("unreadCount", conv.unread_count);
    obj.set("isPinned", conv.is_pinned != 0);
    obj.set("isMuted", conv.is_muted != 0);
    obj.set("updatedAt", (double)conv.updated_at_ms);
    return obj;
}

val friendToJS(const AnyChatFriend_C& f) {
    val obj = val::object();
    obj.set("userId", std::string(f.user_id));
    obj.set("remark", std::string(f.remark));
    obj.set("updatedAt", (double)f.updated_at_ms);
    obj.set("isDeleted", f.is_deleted != 0);
    obj.set("userInfo", userInfoToJS(f.user_info));
    return obj;
}

val friendRequestToJS(const AnyChatFriendRequest_C& req) {
    val obj = val::object();
    obj.set("requestId", (double)req.request_id);
    obj.set("fromUserId", std::string(req.from_user_id));
    obj.set("toUserId", std::string(req.to_user_id));
    obj.set("message", std::string(req.message));
    obj.set("status", std::string(req.status));
    obj.set("createdAt", (double)req.created_at_ms);
    obj.set("fromUserInfo", userInfoToJS(req.from_user_info));
    return obj;
}

val groupToJS(const AnyChatGroup_C& g) {
    val obj = val::object();
    obj.set("groupId", std::string(g.group_id));
    obj.set("name", std::string(g.name));
    obj.set("avatarUrl", std::string(g.avatar_url));
    obj.set("ownerId", std::string(g.owner_id));
    obj.set("memberCount", g.member_count);
    obj.set("myRole", g.my_role);
    obj.set("joinVerify", g.join_verify != 0);
    obj.set("updatedAt", (double)g.updated_at_ms);
    return obj;
}

val groupMemberToJS(const AnyChatGroupMember_C& m) {
    val obj = val::object();
    obj.set("userId", std::string(m.user_id));
    obj.set("groupNickname", std::string(m.group_nickname));
    obj.set("role", m.role);
    obj.set("isMuted", m.is_muted != 0);
    obj.set("joinedAt", (double)m.joined_at_ms);
    obj.set("userInfo", userInfoToJS(m.user_info));
    return obj;
}

// ============================================================================
// Callback Storage: JavaScript callbacks passed from TypeScript
// ============================================================================

struct CallbackStore {
    // Connection state callback
    val connectionStateCallback = val::null();

    // Auth callbacks
    std::map<int, val> authCallbacks;
    std::map<int, val> resultCallbacks;
    val authExpiredCallback = val::null();

    // Message callbacks
    std::map<int, val> messageCallbacks;
    std::map<int, val> messageListCallbacks;
    val messageReceivedCallback = val::null();

    // Conversation callbacks
    std::map<int, val> convCallbacks;
    std::map<int, val> convListCallbacks;
    val convUpdatedCallback = val::null();

    // Friend callbacks
    std::map<int, val> friendCallbacks;
    std::map<int, val> friendListCallbacks;
    std::map<int, val> friendRequestListCallbacks;
    val friendRequestCallback = val::null();
    val friendListChangedCallback = val::null();

    // Group callbacks
    std::map<int, val> groupCallbacks;
    std::map<int, val> groupListCallbacks;
    std::map<int, val> groupMemberCallbacks;
    val groupInvitedCallback = val::null();
    val groupUpdatedCallback = val::null();

    int nextCallbackId = 1;
};

static CallbackStore g_callbacks;

// ============================================================================
// C Callback Wrappers
// ============================================================================

void connectionStateCallbackWrapper(void* userdata, int state) {
    if (!g_callbacks.connectionStateCallback.isNull()) {
        g_callbacks.connectionStateCallback(state);
    }
}

void authSuccessCallbackWrapper(void* userdata, const AnyChatAuthToken_C* token) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.authCallbacks.find(callbackId);
    if (it != g_callbacks.authCallbacks.end()) {
        val callback = it->second;
        if (token) {
            callback(val::null(), authTokenToJS(*token));
        } else {
            callback(std::string("Unknown error"), val::null());
        }
        g_callbacks.authCallbacks.erase(it);
    }
}

void authErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.authCallbacks.find(callbackId);
    if (it != g_callbacks.authCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"), val::null());
        g_callbacks.authCallbacks.erase(it);
    }
}

void resultSuccessCallbackWrapper(void* userdata) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.resultCallbacks.find(callbackId);
    if (it != g_callbacks.resultCallbacks.end()) {
        it->second(val::null());
        g_callbacks.resultCallbacks.erase(it);
    }
}

void resultErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.resultCallbacks.find(callbackId);
    if (it != g_callbacks.resultCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"));
        g_callbacks.resultCallbacks.erase(it);
    }
}

void authExpiredCallbackWrapper(void* userdata) {
    if (!g_callbacks.authExpiredCallback.isNull()) {
        g_callbacks.authExpiredCallback();
    }
}

void messageSuccessCallbackWrapper(void* userdata) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.messageCallbacks.find(callbackId);
    if (it != g_callbacks.messageCallbacks.end()) {
        it->second(val::null());
        g_callbacks.messageCallbacks.erase(it);
    }
}

void messageErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.messageCallbacks.find(callbackId);
    if (it != g_callbacks.messageCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"));
        g_callbacks.messageCallbacks.erase(it);
    }
}

void messageListSuccessCallbackWrapper(void* userdata, const AnyChatMessageList_C* list) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.messageListCallbacks.find(callbackId);
    if (it != g_callbacks.messageListCallbacks.end()) {
        val callback = it->second;
        if (list) {
            val arr = val::array();
            for (int i = 0; i < list->count; ++i) {
                arr.call<void>("push", messageToJS(list->items[i]));
            }
            callback(val::null(), arr);
        } else {
            callback(val::null(), val::array());
        }
        g_callbacks.messageListCallbacks.erase(it);
    }
}

void messageListErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.messageListCallbacks.find(callbackId);
    if (it != g_callbacks.messageListCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"), val::null());
        g_callbacks.messageListCallbacks.erase(it);
    }
}

void messageReceivedCallbackWrapper(void* userdata, const AnyChatMessage_C* message) {
    if (!g_callbacks.messageReceivedCallback.isNull() && message) {
        g_callbacks.messageReceivedCallback(messageToJS(*message));
    }
}

void convListSuccessCallbackWrapper(void* userdata, const AnyChatConversationList_C* list) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.convListCallbacks.find(callbackId);
    if (it != g_callbacks.convListCallbacks.end()) {
        val callback = it->second;
        if (list) {
            val arr = val::array();
            for (int i = 0; i < list->count; ++i) {
                arr.call<void>("push", conversationToJS(list->items[i]));
            }
            callback(val::null(), arr);
        } else {
            callback(val::null(), val::array());
        }
        g_callbacks.convListCallbacks.erase(it);
    }
}

void convListErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.convListCallbacks.find(callbackId);
    if (it != g_callbacks.convListCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"), val::null());
        g_callbacks.convListCallbacks.erase(it);
    }
}

void convSuccessCallbackWrapper(void* userdata) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.convCallbacks.find(callbackId);
    if (it != g_callbacks.convCallbacks.end()) {
        it->second(val::null());
        g_callbacks.convCallbacks.erase(it);
    }
}

void convErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.convCallbacks.find(callbackId);
    if (it != g_callbacks.convCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"));
        g_callbacks.convCallbacks.erase(it);
    }
}

void convUpdatedCallbackWrapper(void* userdata, const AnyChatConversation_C* conversation) {
    if (!g_callbacks.convUpdatedCallback.isNull() && conversation) {
        g_callbacks.convUpdatedCallback(conversationToJS(*conversation));
    }
}

void friendListSuccessCallbackWrapper(void* userdata, const AnyChatFriendList_C* list) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.friendListCallbacks.find(callbackId);
    if (it != g_callbacks.friendListCallbacks.end()) {
        val callback = it->second;
        if (list) {
            val arr = val::array();
            for (int i = 0; i < list->count; ++i) {
                arr.call<void>("push", friendToJS(list->items[i]));
            }
            callback(val::null(), arr);
        } else {
            callback(val::null(), val::array());
        }
        g_callbacks.friendListCallbacks.erase(it);
    }
}

void friendListErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.friendListCallbacks.find(callbackId);
    if (it != g_callbacks.friendListCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"), val::null());
        g_callbacks.friendListCallbacks.erase(it);
    }
}

void friendRequestListSuccessCallbackWrapper(void* userdata, const AnyChatFriendRequestList_C* list) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.friendRequestListCallbacks.find(callbackId);
    if (it != g_callbacks.friendRequestListCallbacks.end()) {
        val callback = it->second;
        if (list) {
            val arr = val::array();
            for (int i = 0; i < list->count; ++i) {
                arr.call<void>("push", friendRequestToJS(list->items[i]));
            }
            callback(val::null(), arr);
        } else {
            callback(val::null(), val::array());
        }
        g_callbacks.friendRequestListCallbacks.erase(it);
    }
}

void friendRequestListErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.friendRequestListCallbacks.find(callbackId);
    if (it != g_callbacks.friendRequestListCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"), val::null());
        g_callbacks.friendRequestListCallbacks.erase(it);
    }
}

void friendSuccessCallbackWrapper(void* userdata) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.friendCallbacks.find(callbackId);
    if (it != g_callbacks.friendCallbacks.end()) {
        it->second(val::null());
        g_callbacks.friendCallbacks.erase(it);
    }
}

void friendErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.friendCallbacks.find(callbackId);
    if (it != g_callbacks.friendCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"));
        g_callbacks.friendCallbacks.erase(it);
    }
}

void friendRequestCallbackWrapper(void* userdata, const AnyChatFriendRequest_C* request) {
    if (!g_callbacks.friendRequestCallback.isNull() && request) {
        g_callbacks.friendRequestCallback(friendRequestToJS(*request));
    }
}

void friendListChangedFromFriendWrapper(void* userdata, const AnyChatFriend_C* friend_info) {
    if (!g_callbacks.friendListChangedCallback.isNull()) {
        g_callbacks.friendListChangedCallback();
    }
}

void friendListChangedFromDeletedWrapper(void* userdata, const char* user_id) {
    if (!g_callbacks.friendListChangedCallback.isNull()) {
        g_callbacks.friendListChangedCallback();
    }
}

void friendListChangedFromRequestWrapper(void* userdata, const AnyChatFriendRequest_C* request) {
    if (!g_callbacks.friendListChangedCallback.isNull()) {
        g_callbacks.friendListChangedCallback();
    }
}

void groupListSuccessCallbackWrapper(void* userdata, const AnyChatGroupList_C* list) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.groupListCallbacks.find(callbackId);
    if (it != g_callbacks.groupListCallbacks.end()) {
        val callback = it->second;
        if (list) {
            val arr = val::array();
            for (int i = 0; i < list->count; ++i) {
                arr.call<void>("push", groupToJS(list->items[i]));
            }
            callback(val::null(), arr);
        } else {
            callback(val::null(), val::array());
        }
        g_callbacks.groupListCallbacks.erase(it);
    }
}

void groupListErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.groupListCallbacks.find(callbackId);
    if (it != g_callbacks.groupListCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"), val::null());
        g_callbacks.groupListCallbacks.erase(it);
    }
}

void groupSuccessCallbackWrapper(void* userdata) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.groupCallbacks.find(callbackId);
    if (it != g_callbacks.groupCallbacks.end()) {
        it->second(val::null());
        g_callbacks.groupCallbacks.erase(it);
    }
}

void groupErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.groupCallbacks.find(callbackId);
    if (it != g_callbacks.groupCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"));
        g_callbacks.groupCallbacks.erase(it);
    }
}

void groupInfoSuccessCallbackWrapper(void* userdata, const AnyChatGroup_C* group) {
    groupSuccessCallbackWrapper(userdata);
}

void groupMemberListSuccessCallbackWrapper(void* userdata, const AnyChatGroupMemberList_C* list) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.groupMemberCallbacks.find(callbackId);
    if (it != g_callbacks.groupMemberCallbacks.end()) {
        val callback = it->second;
        if (list) {
            val arr = val::array();
            for (int i = 0; i < list->count; ++i) {
                arr.call<void>("push", groupMemberToJS(list->items[i]));
            }
            callback(val::null(), arr);
        } else {
            callback(val::null(), val::array());
        }
        g_callbacks.groupMemberCallbacks.erase(it);
    }
}

void groupMemberListErrorCallbackWrapper(void* userdata, int code, const char* error) {
    int callbackId = reinterpret_cast<intptr_t>(userdata);
    auto it = g_callbacks.groupMemberCallbacks.find(callbackId);
    if (it != g_callbacks.groupMemberCallbacks.end()) {
        it->second(std::string(error ? error : "Unknown error"), val::null());
        g_callbacks.groupMemberCallbacks.erase(it);
    }
}

void groupInvitedCallbackWrapper(void* userdata, const AnyChatGroup_C* group, const char* inviter_id) {
    if (!g_callbacks.groupInvitedCallback.isNull() && group) {
        g_callbacks.groupInvitedCallback(groupToJS(*group), std::string(inviter_id ? inviter_id : ""));
    }
}

void groupUpdatedCallbackWrapper(void* userdata, const AnyChatGroup_C* group) {
    if (!g_callbacks.groupUpdatedCallback.isNull() && group) {
        g_callbacks.groupUpdatedCallback(groupToJS(*group));
    }
}

// ============================================================================
// Embind Wrapper Class
// ============================================================================

class AnyChatClientWrapper {
private:
    AnyChatClientHandle handle_;
    AnyChatAuthHandle authHandle_;
    AnyChatMessageHandle messageHandle_;
    AnyChatConvHandle convHandle_;
    AnyChatFriendHandle friendHandle_;
    AnyChatGroupHandle groupHandle_;

    void refreshAuthListener() {
        if (g_callbacks.authExpiredCallback.isNull()) {
            anychat_auth_set_listener(authHandle_, nullptr);
            return;
        }

        AnyChatAuthListener_C listener{};
        listener.struct_size = sizeof(listener);
        listener.on_auth_expired = authExpiredCallbackWrapper;
        anychat_auth_set_listener(authHandle_, &listener);
    }

    void refreshMessageListener() {
        if (g_callbacks.messageReceivedCallback.isNull()) {
            anychat_message_set_listener(messageHandle_, nullptr);
            return;
        }

        AnyChatMessageListener_C listener{};
        listener.struct_size = sizeof(listener);
        listener.on_message_received = messageReceivedCallbackWrapper;
        anychat_message_set_listener(messageHandle_, &listener);
    }

    void refreshConversationListener() {
        if (g_callbacks.convUpdatedCallback.isNull()) {
            anychat_conv_set_listener(convHandle_, nullptr);
            return;
        }

        AnyChatConvListener_C listener{};
        listener.struct_size = sizeof(listener);
        listener.on_conversation_updated = convUpdatedCallbackWrapper;
        anychat_conv_set_listener(convHandle_, &listener);
    }

    void refreshFriendListener() {
        if (g_callbacks.friendRequestCallback.isNull() && g_callbacks.friendListChangedCallback.isNull()) {
            anychat_friend_set_listener(friendHandle_, nullptr);
            return;
        }

        AnyChatFriendListener_C listener{};
        listener.struct_size = sizeof(listener);
        if (!g_callbacks.friendRequestCallback.isNull()) {
            listener.on_friend_request_received = friendRequestCallbackWrapper;
        }
        if (!g_callbacks.friendListChangedCallback.isNull()) {
            listener.on_friend_added = friendListChangedFromFriendWrapper;
            listener.on_friend_deleted = friendListChangedFromDeletedWrapper;
            listener.on_friend_info_changed = friendListChangedFromFriendWrapper;
            listener.on_friend_request_deleted = friendListChangedFromRequestWrapper;
            listener.on_friend_request_accepted = friendListChangedFromRequestWrapper;
            listener.on_friend_request_rejected = friendListChangedFromRequestWrapper;
        }
        anychat_friend_set_listener(friendHandle_, &listener);
    }

    void refreshGroupListener() {
        if (g_callbacks.groupInvitedCallback.isNull() && g_callbacks.groupUpdatedCallback.isNull()) {
            anychat_group_set_listener(groupHandle_, nullptr);
            return;
        }

        AnyChatGroupListener_C listener{};
        listener.struct_size = sizeof(listener);
        if (!g_callbacks.groupInvitedCallback.isNull()) {
            listener.on_group_invited = groupInvitedCallbackWrapper;
        }
        if (!g_callbacks.groupUpdatedCallback.isNull()) {
            listener.on_group_updated = groupUpdatedCallbackWrapper;
        }
        anychat_group_set_listener(groupHandle_, &listener);
    }

public:
    AnyChatClientWrapper(val config) {
        AnyChatClientConfig_C cfg;
        std::memset(&cfg, 0, sizeof(cfg));

        // Extract config from JavaScript object
        std::string gatewayUrl = config["gatewayUrl"].as<std::string>();
        std::string apiBaseUrl = config["apiBaseUrl"].as<std::string>();
        std::string deviceId = config["deviceId"].as<std::string>();
        std::string dbPath = config.hasOwnProperty("dbPath") ? config["dbPath"].as<std::string>() : ":memory:";

        cfg.gateway_url = gatewayUrl.c_str();
        cfg.api_base_url = apiBaseUrl.c_str();
        cfg.device_id = deviceId.c_str();
        cfg.db_path = dbPath.c_str();
        cfg.connect_timeout_ms = config.hasOwnProperty("connectTimeoutMs") ? config["connectTimeoutMs"].as<int>() : 10000;
        cfg.max_reconnect_attempts = config.hasOwnProperty("maxReconnectAttempts") ? config["maxReconnectAttempts"].as<int>() : 5;
        cfg.auto_reconnect = config.hasOwnProperty("autoReconnect") ? (config["autoReconnect"].as<bool>() ? 1 : 0) : 1;

        handle_ = anychat_client_create(&cfg);
        if (!handle_) {
            throw std::runtime_error("Failed to create client");
        }

        authHandle_ = anychat_client_get_auth(handle_);
        messageHandle_ = anychat_client_get_message(handle_);
        convHandle_ = anychat_client_get_conversation(handle_);
        friendHandle_ = anychat_client_get_friend(handle_);
        groupHandle_ = anychat_client_get_group(handle_);
    }

    ~AnyChatClientWrapper() {
        if (handle_) {
            anychat_client_destroy(handle_);
        }
    }

    int getConnectionState() {
        return anychat_client_get_connection_state(handle_);
    }

    void setConnectionCallback(val callback) {
        g_callbacks.connectionStateCallback = callback;
        if (callback.isNull() || callback.isUndefined()) {
            anychat_client_set_connection_callback(handle_, nullptr, nullptr);
            return;
        }
        anychat_client_set_connection_callback(handle_, nullptr, connectionStateCallbackWrapper);
    }

    // Auth methods
    void login(std::string account, std::string password, std::string deviceType, std::string clientVersion, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.authCallbacks[callbackId] = callback;

        AnyChatAuthTokenCallback_C authCallback{};
        authCallback.struct_size = sizeof(authCallback);
        authCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        authCallback.on_success = authSuccessCallbackWrapper;
        authCallback.on_error = authErrorCallbackWrapper;

        int result = anychat_client_login(
            handle_,
            account.c_str(),
            password.c_str(),
            deviceType.c_str(),
            clientVersion.empty() ? nullptr : clientVersion.c_str(),
            &authCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.authCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void register_(std::string phoneOrEmail, std::string password, std::string verifyCode,
                   std::string deviceType, std::string nickname, std::string clientVersion, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.authCallbacks[callbackId] = callback;

        AnyChatAuthTokenCallback_C authCallback{};
        authCallback.struct_size = sizeof(authCallback);
        authCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        authCallback.on_success = authSuccessCallbackWrapper;
        authCallback.on_error = authErrorCallbackWrapper;

        int result = anychat_auth_register(
            authHandle_,
            phoneOrEmail.c_str(),
            password.c_str(),
            verifyCode.c_str(),
            deviceType.c_str(),
            nickname.empty() ? nullptr : nickname.c_str(),
            clientVersion.empty() ? nullptr : clientVersion.c_str(),
            &authCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.authCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void logout(val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.resultCallbacks[callbackId] = callback;

        AnyChatAuthResultCallback_C resultCallback{};
        resultCallback.struct_size = sizeof(resultCallback);
        resultCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        resultCallback.on_success = resultSuccessCallbackWrapper;
        resultCallback.on_error = resultErrorCallbackWrapper;

        int result = anychat_client_logout(handle_, &resultCallback);

        if (result != ANYCHAT_OK) {
            g_callbacks.resultCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void refreshToken(std::string refreshToken, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.authCallbacks[callbackId] = callback;

        AnyChatAuthTokenCallback_C authCallback{};
        authCallback.struct_size = sizeof(authCallback);
        authCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        authCallback.on_success = authSuccessCallbackWrapper;
        authCallback.on_error = authErrorCallbackWrapper;

        int result = anychat_auth_refresh_token(
            authHandle_,
            refreshToken.c_str(),
            &authCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.authCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    bool isLoggedIn() {
        return anychat_client_is_logged_in(handle_) != 0;
    }

    void setAuthExpiredCallback(val callback) {
        g_callbacks.authExpiredCallback = callback;
        refreshAuthListener();
    }

    // Message methods
    void sendTextMessage(std::string sessionId, std::string content, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.messageCallbacks[callbackId] = callback;

        AnyChatMessageCallback_C messageCallback{};
        messageCallback.struct_size = sizeof(messageCallback);
        messageCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        messageCallback.on_success = messageSuccessCallbackWrapper;
        messageCallback.on_error = messageErrorCallbackWrapper;

        int result = anychat_message_send_text(
            messageHandle_,
            sessionId.c_str(),
            content.c_str(),
            &messageCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.messageCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void getMessageHistory(std::string sessionId, double beforeTimestamp, int limit, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.messageListCallbacks[callbackId] = callback;

        AnyChatMessageListCallback_C listCallback{};
        listCallback.struct_size = sizeof(listCallback);
        listCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        listCallback.on_success = messageListSuccessCallbackWrapper;
        listCallback.on_error = messageListErrorCallbackWrapper;

        int result = anychat_message_get_history(
            messageHandle_,
            sessionId.c_str(),
            static_cast<int64_t>(beforeTimestamp),
            limit,
            &listCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.messageListCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void markMessageRead(std::string sessionId, std::string messageId, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.messageCallbacks[callbackId] = callback;

        AnyChatMessageCallback_C messageCallback{};
        messageCallback.struct_size = sizeof(messageCallback);
        messageCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        messageCallback.on_success = messageSuccessCallbackWrapper;
        messageCallback.on_error = messageErrorCallbackWrapper;

        int result = anychat_message_mark_read(
            messageHandle_,
            sessionId.c_str(),
            messageId.c_str(),
            &messageCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.messageCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void setMessageReceivedCallback(val callback) {
        g_callbacks.messageReceivedCallback = callback;
        refreshMessageListener();
    }

    // Conversation methods
    void getConversationList(val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.convListCallbacks[callbackId] = callback;

        AnyChatConvListCallback_C listCallback{};
        listCallback.struct_size = sizeof(listCallback);
        listCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        listCallback.on_success = convListSuccessCallbackWrapper;
        listCallback.on_error = convListErrorCallbackWrapper;

        int result = anychat_conv_get_list(convHandle_, &listCallback);

        if (result != ANYCHAT_OK) {
            g_callbacks.convListCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void markConversationRead(std::string convId, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.convCallbacks[callbackId] = callback;

        AnyChatConvCallback_C convCallback{};
        convCallback.struct_size = sizeof(convCallback);
        convCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        convCallback.on_success = convSuccessCallbackWrapper;
        convCallback.on_error = convErrorCallbackWrapper;

        int result = anychat_conv_mark_all_read(convHandle_, convId.c_str(), &convCallback);
        if (result != ANYCHAT_OK) {
            g_callbacks.convCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void setConversationPinned(std::string convId, bool pinned, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.convCallbacks[callbackId] = callback;

        AnyChatConvCallback_C convCallback{};
        convCallback.struct_size = sizeof(convCallback);
        convCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        convCallback.on_success = convSuccessCallbackWrapper;
        convCallback.on_error = convErrorCallbackWrapper;

        int result = anychat_conv_set_pinned(
            convHandle_,
            convId.c_str(),
            pinned ? 1 : 0,
            &convCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.convCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void setConversationMuted(std::string convId, bool muted, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.convCallbacks[callbackId] = callback;

        AnyChatConvCallback_C convCallback{};
        convCallback.struct_size = sizeof(convCallback);
        convCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        convCallback.on_success = convSuccessCallbackWrapper;
        convCallback.on_error = convErrorCallbackWrapper;

        int result = anychat_conv_set_muted(
            convHandle_,
            convId.c_str(),
            muted ? 1 : 0,
            &convCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.convCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void deleteConversation(std::string convId, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.convCallbacks[callbackId] = callback;

        AnyChatConvCallback_C convCallback{};
        convCallback.struct_size = sizeof(convCallback);
        convCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        convCallback.on_success = convSuccessCallbackWrapper;
        convCallback.on_error = convErrorCallbackWrapper;

        int result = anychat_conv_delete(
            convHandle_,
            convId.c_str(),
            &convCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.convCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void setConversationUpdatedCallback(val callback) {
        g_callbacks.convUpdatedCallback = callback;
        refreshConversationListener();
    }

    // Friend methods
    void getFriendList(val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.friendListCallbacks[callbackId] = callback;

        AnyChatFriendListCallback_C listCallback{};
        listCallback.struct_size = sizeof(listCallback);
        listCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        listCallback.on_success = friendListSuccessCallbackWrapper;
        listCallback.on_error = friendListErrorCallbackWrapper;

        int result = anychat_friend_get_list(friendHandle_, &listCallback);

        if (result != ANYCHAT_OK) {
            g_callbacks.friendListCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void sendFriendRequest(std::string toUserId, std::string message, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.friendCallbacks[callbackId] = callback;

        AnyChatFriendCallback_C friendCallback{};
        friendCallback.struct_size = sizeof(friendCallback);
        friendCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        friendCallback.on_success = friendSuccessCallbackWrapper;
        friendCallback.on_error = friendErrorCallbackWrapper;

        int result = anychat_friend_add(
            friendHandle_,
            toUserId.c_str(),
            message.c_str(),
            nullptr,
            &friendCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.friendCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void handleFriendRequest(double requestId, bool accept, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.friendCallbacks[callbackId] = callback;

        AnyChatFriendCallback_C friendCallback{};
        friendCallback.struct_size = sizeof(friendCallback);
        friendCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        friendCallback.on_success = friendSuccessCallbackWrapper;
        friendCallback.on_error = friendErrorCallbackWrapper;

        int result = accept
            ? anychat_friend_accept_request(friendHandle_, static_cast<int64_t>(requestId), &friendCallback)
            : anychat_friend_reject_request(friendHandle_, static_cast<int64_t>(requestId), &friendCallback);

        if (result != ANYCHAT_OK) {
            g_callbacks.friendCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void getPendingFriendRequests(val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.friendRequestListCallbacks[callbackId] = callback;

        AnyChatFriendRequestListCallback_C requestListCallback{};
        requestListCallback.struct_size = sizeof(requestListCallback);
        requestListCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        requestListCallback.on_success = friendRequestListSuccessCallbackWrapper;
        requestListCallback.on_error = friendRequestListErrorCallbackWrapper;

        int result = anychat_friend_get_requests(friendHandle_, "received", &requestListCallback);
        if (result != ANYCHAT_OK) {
            g_callbacks.friendRequestListCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void deleteFriend(std::string friendId, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.friendCallbacks[callbackId] = callback;

        AnyChatFriendCallback_C friendCallback{};
        friendCallback.struct_size = sizeof(friendCallback);
        friendCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        friendCallback.on_success = friendSuccessCallbackWrapper;
        friendCallback.on_error = friendErrorCallbackWrapper;

        int result = anychat_friend_delete(
            friendHandle_,
            friendId.c_str(),
            &friendCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.friendCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void setFriendRequestCallback(val callback) {
        g_callbacks.friendRequestCallback = callback;
        refreshFriendListener();
    }

    void setFriendListChangedCallback(val callback) {
        g_callbacks.friendListChangedCallback = callback;
        refreshFriendListener();
    }

    // Group methods
    void getGroupList(val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.groupListCallbacks[callbackId] = callback;

        AnyChatGroupListCallback_C listCallback{};
        listCallback.struct_size = sizeof(listCallback);
        listCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        listCallback.on_success = groupListSuccessCallbackWrapper;
        listCallback.on_error = groupListErrorCallbackWrapper;

        int result = anychat_group_get_list(groupHandle_, &listCallback);

        if (result != ANYCHAT_OK) {
            g_callbacks.groupListCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void createGroup(std::string name, val memberIds, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.groupCallbacks[callbackId] = callback;

        AnyChatGroupInfoCallback_C groupCallback{};
        groupCallback.struct_size = sizeof(groupCallback);
        groupCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        groupCallback.on_success = groupInfoSuccessCallbackWrapper;
        groupCallback.on_error = groupErrorCallbackWrapper;

        // Convert JavaScript array to C array
        std::vector<std::string> members;
        std::vector<const char*> memberPtrs;
        int length = memberIds["length"].as<int>();
        for (int i = 0; i < length; ++i) {
            members.push_back(memberIds[i].as<std::string>());
            memberPtrs.push_back(members.back().c_str());
        }
        memberPtrs.push_back(nullptr);

        int result = anychat_group_create(
            groupHandle_,
            name.c_str(),
            memberPtrs.data(),
            length,
            &groupCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.groupCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void joinGroup(std::string groupId, std::string message, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.groupCallbacks[callbackId] = callback;

        AnyChatGroupCallback_C groupCallback{};
        groupCallback.struct_size = sizeof(groupCallback);
        groupCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        groupCallback.on_success = groupSuccessCallbackWrapper;
        groupCallback.on_error = groupErrorCallbackWrapper;

        int result = anychat_group_join(
            groupHandle_,
            groupId.c_str(),
            message.c_str(),
            &groupCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.groupCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void inviteToGroup(std::string groupId, val userIds, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.groupCallbacks[callbackId] = callback;

        AnyChatGroupCallback_C groupCallback{};
        groupCallback.struct_size = sizeof(groupCallback);
        groupCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        groupCallback.on_success = groupSuccessCallbackWrapper;
        groupCallback.on_error = groupErrorCallbackWrapper;

        std::vector<std::string> users;
        std::vector<const char*> userPtrs;
        int length = userIds["length"].as<int>();
        for (int i = 0; i < length; ++i) {
            users.push_back(userIds[i].as<std::string>());
            userPtrs.push_back(users.back().c_str());
        }
        userPtrs.push_back(nullptr);

        int result = anychat_group_invite(
            groupHandle_,
            groupId.c_str(),
            userPtrs.data(),
            length,
            &groupCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.groupCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void quitGroup(std::string groupId, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.groupCallbacks[callbackId] = callback;

        AnyChatGroupCallback_C groupCallback{};
        groupCallback.struct_size = sizeof(groupCallback);
        groupCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        groupCallback.on_success = groupSuccessCallbackWrapper;
        groupCallback.on_error = groupErrorCallbackWrapper;

        int result = anychat_group_quit(
            groupHandle_,
            groupId.c_str(),
            &groupCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.groupCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result));
        }
    }

    void getGroupMembers(std::string groupId, int page, int pageSize, val callback) {
        int callbackId = g_callbacks.nextCallbackId++;
        g_callbacks.groupMemberCallbacks[callbackId] = callback;

        AnyChatGroupMemberListCallback_C memberListCallback{};
        memberListCallback.struct_size = sizeof(memberListCallback);
        memberListCallback.userdata = reinterpret_cast<void*>(static_cast<intptr_t>(callbackId));
        memberListCallback.on_success = groupMemberListSuccessCallbackWrapper;
        memberListCallback.on_error = groupMemberListErrorCallbackWrapper;

        int result = anychat_group_get_members(
            groupHandle_,
            groupId.c_str(),
            page,
            pageSize,
            &memberListCallback
        );

        if (result != ANYCHAT_OK) {
            g_callbacks.groupMemberCallbacks.erase(callbackId);
            callback(dispatchErrorMessage(result), val::null());
        }
    }

    void setGroupInvitedCallback(val callback) {
        g_callbacks.groupInvitedCallback = callback;
        refreshGroupListener();
    }

    void setGroupUpdatedCallback(val callback) {
        g_callbacks.groupUpdatedCallback = callback;
        refreshGroupListener();
    }
};

// ============================================================================
// Embind Registration
// ============================================================================

EMSCRIPTEN_BINDINGS(anychat) {
    class_<AnyChatClientWrapper>("AnyChatClientWrapper")
        .constructor<val>()
        .function("getConnectionState", &AnyChatClientWrapper::getConnectionState)
        .function("setConnectionCallback", &AnyChatClientWrapper::setConnectionCallback)
        .function("login", &AnyChatClientWrapper::login)
        .function("register", &AnyChatClientWrapper::register_)
        .function("logout", &AnyChatClientWrapper::logout)
        .function("refreshToken", &AnyChatClientWrapper::refreshToken)
        .function("isLoggedIn", &AnyChatClientWrapper::isLoggedIn)
        .function("setAuthExpiredCallback", &AnyChatClientWrapper::setAuthExpiredCallback)
        .function("sendTextMessage", &AnyChatClientWrapper::sendTextMessage)
        .function("getMessageHistory", &AnyChatClientWrapper::getMessageHistory)
        .function("markMessageRead", &AnyChatClientWrapper::markMessageRead)
        .function("setMessageReceivedCallback", &AnyChatClientWrapper::setMessageReceivedCallback)
        .function("getConversationList", &AnyChatClientWrapper::getConversationList)
        .function("markConversationRead", &AnyChatClientWrapper::markConversationRead)
        .function("setConversationPinned", &AnyChatClientWrapper::setConversationPinned)
        .function("setConversationMuted", &AnyChatClientWrapper::setConversationMuted)
        .function("deleteConversation", &AnyChatClientWrapper::deleteConversation)
        .function("setConversationUpdatedCallback", &AnyChatClientWrapper::setConversationUpdatedCallback)
        .function("getFriendList", &AnyChatClientWrapper::getFriendList)
        .function("sendFriendRequest", &AnyChatClientWrapper::sendFriendRequest)
        .function("handleFriendRequest", &AnyChatClientWrapper::handleFriendRequest)
        .function("getPendingFriendRequests", &AnyChatClientWrapper::getPendingFriendRequests)
        .function("deleteFriend", &AnyChatClientWrapper::deleteFriend)
        .function("setFriendRequestCallback", &AnyChatClientWrapper::setFriendRequestCallback)
        .function("setFriendListChangedCallback", &AnyChatClientWrapper::setFriendListChangedCallback)
        .function("getGroupList", &AnyChatClientWrapper::getGroupList)
        .function("createGroup", &AnyChatClientWrapper::createGroup)
        .function("joinGroup", &AnyChatClientWrapper::joinGroup)
        .function("inviteToGroup", &AnyChatClientWrapper::inviteToGroup)
        .function("quitGroup", &AnyChatClientWrapper::quitGroup)
        .function("getGroupMembers", &AnyChatClientWrapper::getGroupMembers)
        .function("setGroupInvitedCallback", &AnyChatClientWrapper::setGroupInvitedCallback)
        .function("setGroupUpdatedCallback", &AnyChatClientWrapper::setGroupUpdatedCallback)
        ;

    // Constants
    constant("STATE_DISCONNECTED", ANYCHAT_STATE_DISCONNECTED);
    constant("STATE_CONNECTING", ANYCHAT_STATE_CONNECTING);
    constant("STATE_CONNECTED", ANYCHAT_STATE_CONNECTED);
    constant("STATE_RECONNECTING", ANYCHAT_STATE_RECONNECTING);

    constant("MSG_TEXT", ANYCHAT_MSG_TEXT);
    constant("MSG_IMAGE", ANYCHAT_MSG_IMAGE);
    constant("MSG_FILE", ANYCHAT_MSG_FILE);
    constant("MSG_AUDIO", ANYCHAT_MSG_AUDIO);
    constant("MSG_VIDEO", ANYCHAT_MSG_VIDEO);

    constant("CONV_PRIVATE", ANYCHAT_CONV_PRIVATE);
    constant("CONV_GROUP", ANYCHAT_CONV_GROUP);
}
