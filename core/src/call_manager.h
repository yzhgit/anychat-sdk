#pragma once

#include "notification_manager.h"

#include "internal/call.h"

#include "network/http_client.h"

#include <memory>
#include <mutex>
#include <string>

namespace anychat {

class CallManagerImpl : public CallManager {
public:
    CallManagerImpl(std::shared_ptr<network::HttpClient> http, NotificationManager* notif_mgr);

    void initiateCall(const std::string& callee_id, CallType type, AnyChatValueCallback<CallSession> callback) override;
    void joinCall(const std::string& call_id, AnyChatValueCallback<CallSession> callback) override;
    void rejectCall(const std::string& call_id, AnyChatCallback callback) override;
    void endCall(const std::string& call_id, AnyChatCallback callback) override;
    void getCallSession(const std::string& call_id, AnyChatValueCallback<CallSession> callback) override;
    void getCallLogs(int page, int page_size, AnyChatValueCallback<CallLogResult> callback) override;

    void
    createMeeting(
        const std::string& title,
        const std::string& password,
        int max_participants,
        AnyChatValueCallback<MeetingRoom> callback
    )
        override;
    void joinMeeting(
        const std::string& room_id,
        const std::string& password,
        AnyChatValueCallback<MeetingRoom> callback
    )
        override;
    void endMeeting(const std::string& room_id, AnyChatCallback callback) override;
    void getMeeting(const std::string& room_id, AnyChatValueCallback<MeetingRoom> callback) override;
    void listMeetings(int page, int page_size, AnyChatValueCallback<MeetingListResult> callback) override;

    void setListener(std::shared_ptr<CallListener> listener) override;

private:
    void handleCallNotification(const NotificationEvent& event);

    std::shared_ptr<network::HttpClient> http_;

    std::mutex handler_mutex_;
    std::shared_ptr<CallListener> listener_;
};

} // namespace anychat
