#include "handles_c.h"
#include "anychat_c/group_c.h"
#include "utils_c.h"

#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void userInfoToC(const anychat::UserInfo& src, AnyChatUserInfo_C* dst) {
    anychat_strlcpy(dst->user_id,    src.user_id.c_str(),    sizeof(dst->user_id));
    anychat_strlcpy(dst->username,   src.username.c_str(),   sizeof(dst->username));
    anychat_strlcpy(dst->avatar_url, src.avatar_url.c_str(), sizeof(dst->avatar_url));
}

void groupToC(const anychat::Group& src, AnyChatGroup_C* dst) {
    anychat_strlcpy(dst->group_id,   src.group_id.c_str(),   sizeof(dst->group_id));
    anychat_strlcpy(dst->name,       src.name.c_str(),       sizeof(dst->name));
    anychat_strlcpy(dst->avatar_url, src.avatar_url.c_str(), sizeof(dst->avatar_url));
    anychat_strlcpy(dst->owner_id,   src.owner_id.c_str(),   sizeof(dst->owner_id));
    dst->member_count  = src.member_count;
    dst->my_role       = static_cast<int>(src.my_role);
    dst->join_verify   = src.join_verify ? 1 : 0;
    dst->updated_at_ms = src.updated_at_ms;
}

void memberToC(const anychat::GroupMember& src, AnyChatGroupMember_C* dst) {
    anychat_strlcpy(dst->user_id,        src.user_id.c_str(),        sizeof(dst->user_id));
    anychat_strlcpy(dst->group_nickname, src.group_nickname.c_str(), sizeof(dst->group_nickname));
    dst->role         = static_cast<int>(src.role);
    dst->is_muted     = src.is_muted ? 1 : 0;
    dst->joined_at_ms = src.joined_at_ms;
    userInfoToC(src.user_info, &dst->user_info);
}

struct GroupCbState {
    std::mutex                  invited_mutex;
    void*                       invited_userdata = nullptr;
    AnyChatGroupInvitedCallback invited_cb       = nullptr;

    std::mutex                  updated_mutex;
    void*                       updated_userdata = nullptr;
    AnyChatGroupUpdatedCallback updated_cb       = nullptr;
};

} // namespace

static std::mutex g_group_cb_map_mutex;
static std::unordered_map<anychat::GroupManager*, GroupCbState*> g_group_cb_map;

static GroupCbState* getOrCreateGroupState(anychat::GroupManager* impl) {
    std::lock_guard<std::mutex> lock(g_group_cb_map_mutex);
    auto it = g_group_cb_map.find(impl);
    if (it != g_group_cb_map.end()) return it->second;
    auto* s = new GroupCbState();
    g_group_cb_map[impl] = s;
    return s;
}

extern "C" {

int anychat_group_get_list(
    AnyChatGroupHandle       handle,
    void*                    userdata,
    AnyChatGroupListCallback callback)
{
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->getList(
        [userdata, callback](std::vector<anychat::Group> list, std::string err) {
            if (!callback) return;
            int count = static_cast<int>(list.size());
            AnyChatGroupList_C c_list{};
            c_list.count = count;
            c_list.items = count > 0
                ? static_cast<AnyChatGroup_C*>(std::calloc(count, sizeof(AnyChatGroup_C)))
                : nullptr;
            for (int i = 0; i < count; ++i) groupToC(list[i], &c_list.items[i]);
            callback(userdata, &c_list, err.empty() ? nullptr : err.c_str());
            std::free(c_list.items);
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_group_create(
    AnyChatGroupHandle   handle,
    const char*          name,
    const char* const*   member_ids,
    int                  member_count,
    void*                userdata,
    AnyChatGroupCallback callback)
{
    if (!handle || !handle->impl || !name) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    std::vector<std::string> ids;
    if (member_ids) {
        for (int i = 0; i < member_count; ++i)
            if (member_ids[i]) ids.emplace_back(member_ids[i]);
    }
    handle->impl->create(name, ids,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_group_join(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    const char*          message,
    void*                userdata,
    AnyChatGroupCallback callback)
{
    if (!handle || !handle->impl || !group_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->join(group_id, message ? message : "",
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_group_invite(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    const char* const*   user_ids,
    int                  user_count,
    void*                userdata,
    AnyChatGroupCallback callback)
{
    if (!handle || !handle->impl || !group_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    std::vector<std::string> ids;
    if (user_ids) {
        for (int i = 0; i < user_count; ++i)
            if (user_ids[i]) ids.emplace_back(user_ids[i]);
    }
    handle->impl->invite(group_id, ids,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_group_quit(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    void*                userdata,
    AnyChatGroupCallback callback)
{
    if (!handle || !handle->impl || !group_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->quit(group_id,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_group_update(
    AnyChatGroupHandle   handle,
    const char*          group_id,
    const char*          name,
    const char*          avatar_url,
    void*                userdata,
    AnyChatGroupCallback callback)
{
    if (!handle || !handle->impl || !group_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->update(group_id,
        name       ? name       : "",
        avatar_url ? avatar_url : "",
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_group_get_members(
    AnyChatGroupHandle         handle,
    const char*                group_id,
    int                        page,
    int                        page_size,
    void*                      userdata,
    AnyChatGroupMemberCallback callback)
{
    if (!handle || !handle->impl || !group_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->getMembers(group_id, page, page_size,
        [userdata, callback](std::vector<anychat::GroupMember> members, std::string err) {
            if (!callback) return;
            int count = static_cast<int>(members.size());
            AnyChatGroupMemberList_C c_list{};
            c_list.count = count;
            c_list.items = count > 0
                ? static_cast<AnyChatGroupMember_C*>(
                      std::calloc(count, sizeof(AnyChatGroupMember_C)))
                : nullptr;
            for (int i = 0; i < count; ++i) memberToC(members[i], &c_list.items[i]);
            callback(userdata, &c_list, err.empty() ? nullptr : err.c_str());
            std::free(c_list.items);
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

void anychat_group_set_invited_callback(
    AnyChatGroupHandle          handle,
    void*                       userdata,
    AnyChatGroupInvitedCallback callback)
{
    if (!handle || !handle->impl) return;
    GroupCbState* state = getOrCreateGroupState(handle->impl);
    {
        std::lock_guard<std::mutex> lock(state->invited_mutex);
        state->invited_userdata = userdata;
        state->invited_cb       = callback;
    }
    if (callback) {
        handle->impl->setOnGroupInvited(
            [state](const anychat::Group& group, const std::string& inviter_id) {
                AnyChatGroupInvitedCallback cb;
                void* ud;
                {
                    std::lock_guard<std::mutex> lock(state->invited_mutex);
                    cb = state->invited_cb;
                    ud = state->invited_userdata;
                }
                if (!cb) return;
                AnyChatGroup_C c_group{};
                groupToC(group, &c_group);
                cb(ud, &c_group, inviter_id.c_str());
            });
    } else {
        handle->impl->setOnGroupInvited(nullptr);
    }
}

void anychat_group_set_updated_callback(
    AnyChatGroupHandle          handle,
    void*                       userdata,
    AnyChatGroupUpdatedCallback callback)
{
    if (!handle || !handle->impl) return;
    GroupCbState* state = getOrCreateGroupState(handle->impl);
    {
        std::lock_guard<std::mutex> lock(state->updated_mutex);
        state->updated_userdata = userdata;
        state->updated_cb       = callback;
    }
    if (callback) {
        handle->impl->setOnGroupUpdated(
            [state](const anychat::Group& group) {
                AnyChatGroupUpdatedCallback cb;
                void* ud;
                {
                    std::lock_guard<std::mutex> lock(state->updated_mutex);
                    cb = state->updated_cb;
                    ud = state->updated_userdata;
                }
                if (!cb) return;
                AnyChatGroup_C c_group{};
                groupToC(group, &c_group);
                cb(ud, &c_group);
            });
    } else {
        handle->impl->setOnGroupUpdated(nullptr);
    }
}

} // extern "C"
