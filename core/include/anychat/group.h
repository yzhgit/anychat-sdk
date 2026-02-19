#pragma once
#include "types.h"
#include <functional>
#include <string>
#include <vector>

namespace anychat {

using GroupListCallback   = std::function<void(std::vector<Group> list, std::string err)>;
using GroupCallback       = std::function<void(bool ok, std::string err)>;
using GroupMemberCallback = std::function<void(std::vector<GroupMember> members, std::string err)>;
using OnGroupInvited      = std::function<void(const Group& group, const std::string& inviter_id)>;
using OnGroupUpdated      = std::function<void(const Group& group)>;

class GroupManager {
public:
    virtual ~GroupManager() = default;

    virtual void getList(GroupListCallback cb) = 0;

    virtual void create(const std::string& name,
                        const std::vector<std::string>& member_ids,
                        GroupCallback cb) = 0;

    virtual void join(const std::string& group_id,
                      const std::string& message,
                      GroupCallback cb) = 0;

    virtual void invite(const std::string& group_id,
                        const std::vector<std::string>& user_ids,
                        GroupCallback cb) = 0;

    virtual void quit(const std::string& group_id, GroupCallback cb) = 0;

    virtual void update(const std::string& group_id,
                        const std::string& name,
                        const std::string& avatar_url,
                        GroupCallback cb) = 0;

    virtual void getMembers(const std::string& group_id,
                            int page,
                            int page_size,
                            GroupMemberCallback cb) = 0;

    virtual void setOnGroupInvited(OnGroupInvited handler) = 0;
    virtual void setOnGroupUpdated(OnGroupUpdated handler) = 0;
};

} // namespace anychat
