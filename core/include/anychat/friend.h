#pragma once

#include "types.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace anychat {

using FriendListCallback = std::function<void(std::vector<Friend> list, std::string err)>;
using FriendRequestListCallback = std::function<void(std::vector<FriendRequest> list, std::string err)>;
using BlacklistListCallback = std::function<void(std::vector<BlacklistItem> list, std::string err)>;
using FriendCallback = std::function<void(bool ok, std::string err)>;

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

class FriendManager {
public:
    virtual ~FriendManager() = default;

    // Fetches friend list. Uses DB cache; incremental sync via lastUpdateTime stored in metadata.
    virtual void getFriendList(FriendListCallback cb) = 0;

    // Friendship management
    virtual void addFriend(
        const std::string& to_user_id,
        const std::string& message,
        const std::string& source,
        FriendCallback cb
    ) = 0;
    virtual void deleteFriend(const std::string& friend_id, FriendCallback cb) = 0;
    virtual void updateRemark(const std::string& friend_id, const std::string& remark, FriendCallback cb) = 0;

    // Friend requests
    virtual void getFriendRequests(const std::string& request_type, FriendRequestListCallback cb) = 0;
    virtual void acceptFriendRequest(int64_t request_id, FriendCallback cb) = 0;
    virtual void rejectFriendRequest(int64_t request_id, FriendCallback cb) = 0;

    // Blacklist
    virtual void getBlacklist(BlacklistListCallback cb) = 0;
    virtual void addToBlacklist(const std::string& user_id, FriendCallback cb) = 0;
    virtual void removeFromBlacklist(const std::string& user_id, FriendCallback cb) = 0;

    // Listener (fired on incoming WS notifications)
    virtual void setListener(std::shared_ptr<FriendListener> listener) = 0;
};

} // namespace anychat
