#pragma once

#include "callbacks.h"
#include "types.h"

#include <memory>
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

class GroupManager {
public:
    virtual ~GroupManager() = default;

    virtual void getGroupList(AnyChatValueCallback<std::vector<Group>> cb) = 0;

    virtual void getInfo(const std::string& group_id, AnyChatValueCallback<Group> cb) {
        (void) group_id;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void
    create(const std::string& name, const std::vector<std::string>& member_ids, AnyChatValueCallback<Group> cb) = 0;

    virtual void join(const std::string& group_id, const std::string& message, AnyChatCallback cb) = 0;

    virtual void invite(const std::string& group_id, const std::vector<std::string>& user_ids, AnyChatCallback cb) = 0;

    virtual void quit(const std::string& group_id, AnyChatCallback cb) = 0;

    virtual void disband(const std::string& group_id, AnyChatCallback cb) {
        (void) group_id;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void
    update(const std::string& group_id, const std::string& name, const std::string& avatar_url, AnyChatCallback cb)
        = 0;

    virtual void
    getMembers(const std::string& group_id, int page, int page_size, AnyChatValueCallback<std::vector<GroupMember>> cb)
        = 0;

    virtual void removeMember(const std::string& group_id, const std::string& user_id, AnyChatCallback cb) {
        (void) group_id;
        (void) user_id;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void updateMemberRole(
        const std::string& group_id,
        const std::string& user_id,
        int32_t role,
        AnyChatCallback cb
    ) {
        (void) group_id;
        (void) user_id;
        (void) role;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void updateNickname(const std::string& group_id, const std::string& nickname, AnyChatCallback cb) {
        (void) group_id;
        (void) nickname;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void transferOwnership(const std::string& group_id, const std::string& new_owner_id, AnyChatCallback cb) {
        (void) group_id;
        (void) new_owner_id;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void getJoinRequests(
        const std::string& group_id,
        int32_t status,
        AnyChatValueCallback<std::vector<GroupJoinRequest>> cb
    ) {
        (void) group_id;
        (void) status;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void handleJoinRequest(const std::string& group_id, int64_t request_id, bool accept, AnyChatCallback cb) {
        (void) group_id;
        (void) request_id;
        (void) accept;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void getQRCode(const std::string& group_id, AnyChatValueCallback<GroupQRCode> cb) {
        (void) group_id;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void refreshQRCode(const std::string& group_id, AnyChatValueCallback<GroupQRCode> cb) {
        (void) group_id;
        if (cb.on_error) {
            cb.on_error(-1, "unsupported operation");
        }
    }

    virtual void setListener(std::shared_ptr<GroupListener> listener) = 0;
};

} // namespace anychat
