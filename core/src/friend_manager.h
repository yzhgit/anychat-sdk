#pragma once

#include "notification_manager.h"
#include "sdk_callbacks.h"
#include "sdk_types.h"

#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace anychat {

class FriendListener {
public:
    virtual ~FriendListener() = default;

    virtual void onFriendAdded(const Friend& friend_info) = 0;
    virtual void onFriendDeleted(const std::string& user_id) = 0;
    virtual void onFriendInfoChanged(const Friend& friend_info) = 0;
    virtual void onBlacklistAdded(const BlacklistItem& item) = 0;
    virtual void onBlacklistRemoved(const std::string& blocked_user_id) = 0;
    virtual void onFriendRequestReceived(const FriendRequest& req) = 0;
    virtual void onFriendRequestDeleted(const FriendRequest& req) = 0;
    virtual void onFriendRequestAccepted(const FriendRequest& req) = 0;
    virtual void onFriendRequestRejected(const FriendRequest& req) = 0;
};

class FriendManagerImpl {
public:
    FriendManagerImpl(db::Database* db, NotificationManager* notif_mgr, std::shared_ptr<network::HttpClient> http);

    // Fetches friend list. Uses DB cache; incremental sync via lastUpdateTime stored in metadata.
    void getFriendList(AnyChatValueCallback<std::vector<Friend>> cb);

    // Friendship management
    void
    addFriend(
        const std::string& to_user_id,
        const std::string& message,
        int32_t source,
        AnyChatCallback cb
    );
    void deleteFriend(const std::string& friend_id, AnyChatCallback cb);
    void updateRemark(const std::string& friend_id, const std::string& remark, AnyChatCallback cb);

    // Friend requests
    void
    getFriendRequests(int32_t request_type, AnyChatValueCallback<std::vector<FriendRequest>> cb);
    void handleFriendRequest(int64_t request_id, int32_t action, AnyChatCallback cb);

    // Blacklist
    void getBlacklist(AnyChatValueCallback<std::vector<BlacklistItem>> cb);
    void addToBlacklist(const std::string& user_id, AnyChatCallback cb);
    void removeFromBlacklist(const std::string& user_id, AnyChatCallback cb);

    // Listener (fired on incoming WS notifications)
    void setListener(std::shared_ptr<FriendListener> listener);

private:
    void handleFriendNotification(const NotificationEvent& event);

    db::Database* db_;
    NotificationManager* notif_mgr_;
    std::shared_ptr<network::HttpClient> http_;

    mutable std::mutex handler_mutex_;
    std::shared_ptr<FriendListener> listener_;
};

} // namespace anychat
