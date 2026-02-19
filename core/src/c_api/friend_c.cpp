#include "handles_c.h"
#include "anychat_c/friend_c.h"
#include "utils_c.h"

#include <cstdlib>
#include <mutex>
#include <unordered_map>

namespace {

void userInfoToC(const anychat::UserInfo& src, AnyChatUserInfo_C* dst) {
    anychat_strlcpy(dst->user_id,    src.user_id.c_str(),    sizeof(dst->user_id));
    anychat_strlcpy(dst->username,   src.username.c_str(),   sizeof(dst->username));
    anychat_strlcpy(dst->avatar_url, src.avatar_url.c_str(), sizeof(dst->avatar_url));
}

void friendToC(const anychat::Friend& src, AnyChatFriend_C* dst) {
    anychat_strlcpy(dst->user_id, src.user_id.c_str(), sizeof(dst->user_id));
    anychat_strlcpy(dst->remark,  src.remark.c_str(),  sizeof(dst->remark));
    dst->updated_at_ms = src.updated_at_ms;
    dst->is_deleted    = src.is_deleted ? 1 : 0;
    userInfoToC(src.user_info, &dst->user_info);
}

void friendRequestToC(const anychat::FriendRequest& src, AnyChatFriendRequest_C* dst) {
    dst->request_id = src.request_id;
    anychat_strlcpy(dst->from_user_id, src.from_user_id.c_str(), sizeof(dst->from_user_id));
    anychat_strlcpy(dst->to_user_id,   src.to_user_id.c_str(),   sizeof(dst->to_user_id));
    anychat_strlcpy(dst->message,      src.message.c_str(),      sizeof(dst->message));
    anychat_strlcpy(dst->status,       src.status.c_str(),       sizeof(dst->status));
    dst->created_at_ms = src.created_at_ms;
    userInfoToC(src.from_user_info, &dst->from_user_info);
}

struct FriendCbState {
    std::mutex                       request_mutex;
    void*                            request_userdata = nullptr;
    AnyChatFriendRequestCallback     request_cb       = nullptr;

    std::mutex                           changed_mutex;
    void*                                changed_userdata = nullptr;
    AnyChatFriendListChangedCallback     changed_cb       = nullptr;
};

} // namespace

static std::mutex g_friend_cb_map_mutex;
static std::unordered_map<anychat::FriendManager*, FriendCbState*> g_friend_cb_map;

static FriendCbState* getOrCreateFriendState(anychat::FriendManager* impl) {
    std::lock_guard<std::mutex> lock(g_friend_cb_map_mutex);
    auto it = g_friend_cb_map.find(impl);
    if (it != g_friend_cb_map.end()) return it->second;
    auto* s = new FriendCbState();
    g_friend_cb_map[impl] = s;
    return s;
}

extern "C" {

int anychat_friend_get_list(
    AnyChatFriendHandle       handle,
    void*                     userdata,
    AnyChatFriendListCallback callback)
{
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }

    handle->impl->getList(
        [userdata, callback](std::vector<anychat::Friend> list, std::string err) {
            if (!callback) return;
            int count = static_cast<int>(list.size());
            AnyChatFriendList_C c_list{};
            c_list.count = count;
            c_list.items = count > 0
                ? static_cast<AnyChatFriend_C*>(std::calloc(count, sizeof(AnyChatFriend_C)))
                : nullptr;
            for (int i = 0; i < count; ++i) friendToC(list[i], &c_list.items[i]);
            callback(userdata, &c_list, err.empty() ? nullptr : err.c_str());
            std::free(c_list.items);
        });

    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_friend_send_request(
    AnyChatFriendHandle   handle,
    const char*           to_user_id,
    const char*           message,
    void*                 userdata,
    AnyChatFriendCallback callback)
{
    if (!handle || !handle->impl || !to_user_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->sendRequest(to_user_id, message ? message : "",
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_friend_handle_request(
    AnyChatFriendHandle   handle,
    int64_t               request_id,
    int                   accept,
    void*                 userdata,
    AnyChatFriendCallback callback)
{
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->handleRequest(request_id, accept != 0,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_friend_get_pending_requests(
    AnyChatFriendHandle              handle,
    void*                            userdata,
    AnyChatFriendRequestListCallback callback)
{
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->getPendingRequests(
        [userdata, callback](std::vector<anychat::FriendRequest> list, std::string err) {
            if (!callback) return;
            int count = static_cast<int>(list.size());
            AnyChatFriendRequestList_C c_list{};
            c_list.count = count;
            c_list.items = count > 0
                ? static_cast<AnyChatFriendRequest_C*>(
                      std::calloc(count, sizeof(AnyChatFriendRequest_C)))
                : nullptr;
            for (int i = 0; i < count; ++i)
                friendRequestToC(list[i], &c_list.items[i]);
            callback(userdata, &c_list, err.empty() ? nullptr : err.c_str());
            std::free(c_list.items);
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_friend_delete(
    AnyChatFriendHandle   handle,
    const char*           friend_id,
    void*                 userdata,
    AnyChatFriendCallback callback)
{
    if (!handle || !handle->impl || !friend_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->deleteFriend(friend_id,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_friend_update_remark(
    AnyChatFriendHandle   handle,
    const char*           friend_id,
    const char*           remark,
    void*                 userdata,
    AnyChatFriendCallback callback)
{
    if (!handle || !handle->impl || !friend_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->updateRemark(friend_id, remark ? remark : "",
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_friend_add_to_blacklist(
    AnyChatFriendHandle   handle,
    const char*           user_id,
    void*                 userdata,
    AnyChatFriendCallback callback)
{
    if (!handle || !handle->impl || !user_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->addToBlacklist(user_id,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_friend_remove_from_blacklist(
    AnyChatFriendHandle   handle,
    const char*           user_id,
    void*                 userdata,
    AnyChatFriendCallback callback)
{
    if (!handle || !handle->impl || !user_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->removeFromBlacklist(user_id,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

void anychat_friend_set_request_callback(
    AnyChatFriendHandle          handle,
    void*                        userdata,
    AnyChatFriendRequestCallback callback)
{
    if (!handle || !handle->impl) return;
    FriendCbState* state = getOrCreateFriendState(handle->impl);
    {
        std::lock_guard<std::mutex> lock(state->request_mutex);
        state->request_userdata = userdata;
        state->request_cb       = callback;
    }
    if (callback) {
        handle->impl->setOnFriendRequest(
            [state](const anychat::FriendRequest& req) {
                AnyChatFriendRequestCallback cb;
                void* ud;
                {
                    std::lock_guard<std::mutex> lock(state->request_mutex);
                    cb = state->request_cb;
                    ud = state->request_userdata;
                }
                if (!cb) return;
                AnyChatFriendRequest_C c_req{};
                friendRequestToC(req, &c_req);
                cb(ud, &c_req);
            });
    } else {
        handle->impl->setOnFriendRequest(nullptr);
    }
}

void anychat_friend_set_list_changed_callback(
    AnyChatFriendHandle              handle,
    void*                            userdata,
    AnyChatFriendListChangedCallback callback)
{
    if (!handle || !handle->impl) return;
    FriendCbState* state = getOrCreateFriendState(handle->impl);
    {
        std::lock_guard<std::mutex> lock(state->changed_mutex);
        state->changed_userdata = userdata;
        state->changed_cb       = callback;
    }
    if (callback) {
        handle->impl->setOnFriendListChanged(
            [state]() {
                AnyChatFriendListChangedCallback cb;
                void* ud;
                {
                    std::lock_guard<std::mutex> lock(state->changed_mutex);
                    cb = state->changed_cb;
                    ud = state->changed_userdata;
                }
                if (cb) cb(ud);
            });
    } else {
        handle->impl->setOnFriendListChanged(nullptr);
    }
}

} // extern "C"
