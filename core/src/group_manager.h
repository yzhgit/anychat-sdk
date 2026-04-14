#pragma once
#include "notification_manager.h"

#include "internal/group.h"

#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <mutex>

namespace anychat {

class GroupManagerImpl : public GroupManager {
public:
    GroupManagerImpl(db::Database* db, NotificationManager* notif_mgr, std::shared_ptr<network::HttpClient> http);

    // GroupManager interface
    void getGroupList(AnyChatValueCallback<std::vector<Group>> cb) override;
    void getInfo(const std::string& group_id, AnyChatValueCallback<Group> cb) override;
    void create(const std::string& name, const std::vector<std::string>& member_ids, AnyChatValueCallback<Group> cb)
        override;
    void join(const std::string& group_id, const std::string& message, AnyChatCallback cb) override;
    void invite(const std::string& group_id, const std::vector<std::string>& user_ids, AnyChatCallback cb) override;
    void quit(const std::string& group_id, AnyChatCallback cb) override;
    void disband(const std::string& group_id, AnyChatCallback cb) override;
    void update(const std::string& group_id, const std::string& name, const std::string& avatar_url, AnyChatCallback cb)
        override;
    void
    getMembers(const std::string& group_id, int page, int page_size, AnyChatValueCallback<std::vector<GroupMember>> cb)
        override;
    void removeMember(const std::string& group_id, const std::string& user_id, AnyChatCallback cb) override;
    void
    updateMemberRole(const std::string& group_id, const std::string& user_id, GroupRole role, AnyChatCallback cb)
        override;
    void updateNickname(const std::string& group_id, const std::string& nickname, AnyChatCallback cb) override;
    void transferOwnership(const std::string& group_id, const std::string& new_owner_id, AnyChatCallback cb) override;
    void getJoinRequests(
        const std::string& group_id,
        const std::string& status,
        AnyChatValueCallback<std::vector<GroupJoinRequest>> cb
    )
        override;
    void handleJoinRequest(const std::string& group_id, int64_t request_id, bool accept, AnyChatCallback cb) override;
    void getQRCode(const std::string& group_id, AnyChatValueCallback<GroupQRCode> cb) override;
    void refreshQRCode(const std::string& group_id, AnyChatValueCallback<GroupQRCode> cb) override;
    void setListener(std::shared_ptr<GroupListener> listener) override;

private:
    void handleGroupNotification(const NotificationEvent& event);

    db::Database* db_;
    NotificationManager* notif_mgr_;
    std::shared_ptr<network::HttpClient> http_;

    mutable std::mutex handler_mutex_;
    std::shared_ptr<GroupListener> listener_;
};

} // namespace anychat
