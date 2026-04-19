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

class GroupListener {
public:
    virtual ~GroupListener() = default;

    virtual void onGroupInvited(const Group& group, const std::string& inviter_id) {
        (void) group;
        (void) inviter_id;
    }

    virtual void onGroupUpdated(const Group& group) {
        (void) group;
    }
};

class GroupManagerImpl {
public:
    GroupManagerImpl(db::Database* db, NotificationManager* notif_mgr, std::shared_ptr<network::HttpClient> http);

    void getGroupList(AnyChatValueCallback<std::vector<Group>> cb);

    void getInfo(const std::string& group_id, AnyChatValueCallback<Group> cb);

    void
    create(const std::string& name, const std::vector<std::string>& member_ids, AnyChatValueCallback<Group> cb);

    void join(const std::string& group_id, const std::string& message, AnyChatCallback cb);

    void invite(const std::string& group_id, const std::vector<std::string>& user_ids, AnyChatCallback cb);

    void quit(const std::string& group_id, AnyChatCallback cb);

    void disband(const std::string& group_id, AnyChatCallback cb);

    void
    update(const std::string& group_id, const std::string& name, const std::string& avatar_url, AnyChatCallback cb);

    void
    getMembers(const std::string& group_id, int page, int page_size, AnyChatValueCallback<std::vector<GroupMember>> cb);

    void removeMember(const std::string& group_id, const std::string& user_id, AnyChatCallback cb);

    void updateMemberRole(
        const std::string& group_id,
        const std::string& user_id,
        int32_t role,
        AnyChatCallback cb
    );

    void updateNickname(const std::string& group_id, const std::string& nickname, AnyChatCallback cb);

    void transferOwnership(const std::string& group_id, const std::string& new_owner_id, AnyChatCallback cb);

    void getJoinRequests(
        const std::string& group_id,
        int32_t status,
        AnyChatValueCallback<std::vector<GroupJoinRequest>> cb
    );

    void handleJoinRequest(const std::string& group_id, int64_t request_id, bool accept, AnyChatCallback cb);

    void getQRCode(const std::string& group_id, AnyChatValueCallback<GroupQRCode> cb);

    void refreshQRCode(const std::string& group_id, AnyChatValueCallback<GroupQRCode> cb);

    void setListener(std::shared_ptr<GroupListener> listener);

private:
    void handleGroupNotification(const NotificationEvent& event);

    db::Database* db_;
    NotificationManager* notif_mgr_;
    std::shared_ptr<network::HttpClient> http_;

    mutable std::mutex handler_mutex_;
    std::shared_ptr<GroupListener> listener_;
};

} // namespace anychat
