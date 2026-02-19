#pragma once
#include "types.h"
#include <functional>
#include <string>
#include <vector>

namespace anychat {

using FriendListCallback        = std::function<void(std::vector<Friend> list, std::string err)>;
using FriendRequestListCallback = std::function<void(std::vector<FriendRequest> list, std::string err)>;
using FriendCallback            = std::function<void(bool ok, std::string err)>;
using OnFriendRequest           = std::function<void(const FriendRequest& req)>;
using OnFriendListChanged       = std::function<void()>;

class FriendManager {
public:
    virtual ~FriendManager() = default;

    // Fetches friend list. Uses DB cache; incremental sync via lastUpdateTime stored in metadata.
    virtual void getList(FriendListCallback cb) = 0;

    // Friend requests
    virtual void sendRequest(const std::string& to_user_id,
                             const std::string& message,
                             FriendCallback cb) = 0;

    virtual void handleRequest(int64_t request_id, bool accept, FriendCallback cb) = 0;

    virtual void getPendingRequests(FriendRequestListCallback cb) = 0;

    // Friendship management
    virtual void deleteFriend(const std::string& friend_id, FriendCallback cb) = 0;

    virtual void updateRemark(const std::string& friend_id,
                              const std::string& remark,
                              FriendCallback cb) = 0;

    // Blacklist
    virtual void addToBlacklist(const std::string& user_id, FriendCallback cb) = 0;
    virtual void removeFromBlacklist(const std::string& user_id, FriendCallback cb) = 0;

    // Callbacks (fired on incoming WS notifications)
    virtual void setOnFriendRequest(OnFriendRequest handler) = 0;
    virtual void setOnFriendListChanged(OnFriendListChanged handler) = 0;
};

} // namespace anychat
