#include "handles_c.h"
#include "anychat_c/conversation_c.h"
#include "utils_c.h"

#include <cstdlib>
#include <mutex>
#include <unordered_map>

namespace {

void convToCStruct(const anychat::Conversation& src, AnyChatConversation_C* dst) {
    anychat_strlcpy(dst->conv_id,        src.conv_id.c_str(),       sizeof(dst->conv_id));
    anychat_strlcpy(dst->target_id,      src.target_id.c_str(),     sizeof(dst->target_id));
    anychat_strlcpy(dst->last_msg_id,    src.last_msg_id.c_str(),   sizeof(dst->last_msg_id));
    anychat_strlcpy(dst->last_msg_text,  src.last_msg_text.c_str(), sizeof(dst->last_msg_text));
    dst->conv_type        = (src.conv_type == anychat::ConversationType::Private)
                            ? ANYCHAT_CONV_PRIVATE : ANYCHAT_CONV_GROUP;
    dst->last_msg_time_ms = src.last_msg_time_ms;
    dst->unread_count     = src.unread_count;
    dst->is_pinned        = src.is_pinned ? 1 : 0;
    dst->is_muted         = src.is_muted  ? 1 : 0;
    dst->updated_at_ms    = src.updated_at_ms;
}

struct ConvCallbackState {
    std::mutex                 mutex;
    void*                      userdata = nullptr;
    AnyChatConvUpdatedCallback callback = nullptr;
};

} // namespace

static std::mutex g_conv_cb_map_mutex;
static std::unordered_map<anychat::ConversationManager*, ConvCallbackState*> g_conv_cb_map;

static ConvCallbackState* getOrCreateConvState(anychat::ConversationManager* impl) {
    std::lock_guard<std::mutex> lock(g_conv_cb_map_mutex);
    auto it = g_conv_cb_map.find(impl);
    if (it != g_conv_cb_map.end()) return it->second;
    auto* s = new ConvCallbackState();
    g_conv_cb_map[impl] = s;
    return s;
}

extern "C" {

int anychat_conv_get_list(
    AnyChatConvHandle       handle,
    void*                   userdata,
    AnyChatConvListCallback callback)
{
    if (!handle || !handle->impl) {
        anychat_set_last_error("invalid handle");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }

    handle->impl->getList(
        [userdata, callback](std::vector<anychat::Conversation> list,
                             std::string err)
        {
            if (!callback) return;
            int count = static_cast<int>(list.size());
            AnyChatConversationList_C c_list{};
            c_list.count = count;
            c_list.items = count > 0
                ? static_cast<AnyChatConversation_C*>(
                      std::calloc(count, sizeof(AnyChatConversation_C)))
                : nullptr;
            for (int i = 0; i < count; ++i)
                convToCStruct(list[i], &c_list.items[i]);
            callback(userdata, &c_list, err.empty() ? nullptr : err.c_str());
            std::free(c_list.items);
        });

    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_conv_mark_read(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    void*               userdata,
    AnyChatConvCallback callback)
{
    if (!handle || !handle->impl || !conv_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->markRead(conv_id,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_conv_set_pinned(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    int                 pinned,
    void*               userdata,
    AnyChatConvCallback callback)
{
    if (!handle || !handle->impl || !conv_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->setPinned(conv_id, pinned != 0,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_conv_set_muted(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    int                 muted,
    void*               userdata,
    AnyChatConvCallback callback)
{
    if (!handle || !handle->impl || !conv_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->setMuted(conv_id, muted != 0,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

int anychat_conv_delete(
    AnyChatConvHandle   handle,
    const char*         conv_id,
    void*               userdata,
    AnyChatConvCallback callback)
{
    if (!handle || !handle->impl || !conv_id) {
        anychat_set_last_error("invalid arguments");
        return ANYCHAT_ERROR_INVALID_PARAM;
    }
    handle->impl->deleteConv(conv_id,
        [userdata, callback](bool ok, std::string err) {
            if (callback) callback(userdata, ok ? 1 : 0, err.c_str());
        });
    anychat_clear_last_error();
    return ANYCHAT_OK;
}

void anychat_conv_set_updated_callback(
    AnyChatConvHandle          handle,
    void*                      userdata,
    AnyChatConvUpdatedCallback callback)
{
    if (!handle || !handle->impl) return;

    ConvCallbackState* state = getOrCreateConvState(handle->impl);
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->userdata = userdata;
        state->callback = callback;
    }

    if (callback) {
        handle->impl->setOnConversationUpdated(
            [state](const anychat::Conversation& conv) {
                AnyChatConvUpdatedCallback cb;
                void* ud;
                {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    cb = state->callback;
                    ud = state->userdata;
                }
                if (!cb) return;
                AnyChatConversation_C c_conv{};
                convToCStruct(conv, &c_conv);
                cb(ud, &c_conv);
            });
    } else {
        handle->impl->setOnConversationUpdated(nullptr);
    }
}

} // extern "C"
