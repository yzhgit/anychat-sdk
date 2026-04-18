#pragma once
#include "notification_manager.h"

#include "internal/friend.h"

#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <mutex>

namespace anychat {

class FriendManagerImpl : public FriendManager {
public:
    FriendManagerImpl(db::Database* db, NotificationManager* notif_mgr, std::shared_ptr<network::HttpClient> http);

    // FriendManager interface
    void getFriendList(AnyChatValueCallback<std::vector<Friend>> cb) override;
    void
    addFriend(
        const std::string& to_user_id,
        const std::string& message,
        int32_t source,
        AnyChatCallback cb
    )
        override;
    void deleteFriend(const std::string& friend_id, AnyChatCallback cb) override;
    void updateRemark(const std::string& friend_id, const std::string& remark, AnyChatCallback cb) override;

    void getFriendRequests(int32_t request_type, AnyChatValueCallback<std::vector<FriendRequest>> cb)
        override;
    void handleFriendRequest(int64_t request_id, int32_t action, AnyChatCallback cb) override;

    void getBlacklist(AnyChatValueCallback<std::vector<BlacklistItem>> cb) override;
    void addToBlacklist(const std::string& user_id, AnyChatCallback cb) override;
    void removeFromBlacklist(const std::string& user_id, AnyChatCallback cb) override;

    void setListener(std::shared_ptr<FriendListener> listener) override;

private:
    void handleFriendNotification(const NotificationEvent& event);

    db::Database* db_;
    NotificationManager* notif_mgr_;
    std::shared_ptr<network::HttpClient> http_;

    mutable std::mutex handler_mutex_;
    std::shared_ptr<FriendListener> listener_;
};

} // namespace anychat
