#pragma once
#include "anychat/group.h"
#include "notification_manager.h"
#include "db/database.h"
#include "network/http_client.h"
#include <memory>
#include <mutex>

namespace anychat {

class GroupManagerImpl : public GroupManager {
public:
    GroupManagerImpl(db::Database* db,
                     NotificationManager* notif_mgr,
                     std::shared_ptr<network::HttpClient> http);

    // GroupManager interface
    void getList(GroupListCallback cb) override;
    void create(const std::string& name,
                const std::vector<std::string>& member_ids,
                GroupCallback cb) override;
    void join(const std::string& group_id,
              const std::string& message,
              GroupCallback cb) override;
    void invite(const std::string& group_id,
                const std::vector<std::string>& user_ids,
                GroupCallback cb) override;
    void quit(const std::string& group_id, GroupCallback cb) override;
    void update(const std::string& group_id,
                const std::string& name,
                const std::string& avatar_url,
                GroupCallback cb) override;
    void getMembers(const std::string& group_id,
                    int page,
                    int page_size,
                    GroupMemberCallback cb) override;
    void setOnGroupInvited(OnGroupInvited handler) override;
    void setOnGroupUpdated(OnGroupUpdated handler) override;

private:
    void handleGroupNotification(const NotificationEvent& event);

    db::Database*                        db_;
    NotificationManager*                 notif_mgr_;
    std::shared_ptr<network::HttpClient> http_;

    mutable std::mutex  handler_mutex_;
    OnGroupInvited      on_group_invited_;
    OnGroupUpdated      on_group_updated_;
};

} // namespace anychat
