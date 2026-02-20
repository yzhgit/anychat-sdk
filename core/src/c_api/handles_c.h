#pragma once

/* Internal handle struct definitions shared across all c_api implementation files.
 * Not part of the public API. */

#include "anychat/client.h"
#include "anychat/auth.h"
#include "anychat/message.h"
#include "anychat/conversation.h"
#include "anychat/friend.h"
#include "anychat/group.h"
#include "anychat/file.h"
#include "anychat/user.h"
#include "anychat/rtc.h"

#include <memory>
#include <mutex>

/* Forward declare C callback types to avoid circular includes */
typedef void (*AnyChatConnectionStateCallback)(void* userdata, int state);

/* Sub-module handle wrappers (non-owning pointers into the client). */
struct AnyChatAuthManager_T   { anychat::AuthManager*         impl; };
struct AnyChatMessage_T       { anychat::MessageManager*      impl; };
struct AnyChatConversation_T  { anychat::ConversationManager* impl; };
struct AnyChatFriend_T        { anychat::FriendManager*       impl; };
struct AnyChatGroup_T         { anychat::GroupManager*        impl; };
struct AnyChatFile_T          { anychat::FileManager*         impl; };
struct AnyChatUser_T          { anychat::UserManager*         impl; };
struct AnyChatRtc_T           { anychat::RtcManager*          impl; };

/* Main client handle. */
struct AnyChatClient_T {
    std::shared_ptr<anychat::AnyChatClient> impl;

    AnyChatAuthManager_T  auth_handle;
    AnyChatMessage_T      msg_handle;
    AnyChatConversation_T conv_handle;
    AnyChatFriend_T       friend_handle;
    AnyChatGroup_T        group_handle;
    AnyChatFile_T         file_handle;
    AnyChatUser_T         user_handle;
    AnyChatRtc_T          rtc_handle;

    std::mutex                         cb_mutex;
    void*                              cb_userdata = nullptr;
    AnyChatConnectionStateCallback     cb          = nullptr;

    // Error message buffer for async callbacks
    // Used to store error messages that need to persist across async callback boundaries
    std::mutex        error_mutex;
    std::string       last_callback_error;
};
