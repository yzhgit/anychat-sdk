#pragma once

#include "notification_manager.h"
#include "sdk_callbacks.h"
#include "sdk_types.h"

#include "network/http_client.h"

#include <memory>
#include <mutex>
#include <string>

namespace anychat {

class CallListener {
public:
    virtual ~CallListener() = default;

    virtual void onIncomingCall(const CallSession& session) {
        (void) session;
    }

    virtual void onCallStatusChanged(const std::string& call_id, CallStatus status) {
        (void) call_id;
        (void) status;
    }
};

class CallManagerImpl {
public:
    CallManagerImpl(std::shared_ptr<network::HttpClient> http, NotificationManager* notif_mgr);

    // ---- One-to-one calls ------------------------------------------------
    // POST /calling/calls
    void initiateCall(const std::string& callee_id, CallType type, AnyChatValueCallback<CallSession> callback);

    // POST /calling/calls/{callId}/join
    void joinCall(const std::string& call_id, AnyChatValueCallback<CallSession> callback);

    // POST /calling/calls/{callId}/reject
    void rejectCall(const std::string& call_id, AnyChatCallback callback);

    // POST /calling/calls/{callId}/end
    void endCall(const std::string& call_id, AnyChatCallback callback);

    // GET /calling/calls/{callId}
    void getCallSession(const std::string& call_id, AnyChatValueCallback<CallSession> callback);

    // GET /calling/calls?page=&pageSize=
    void getCallLogs(int page, int page_size, AnyChatValueCallback<CallLogResult> callback);

    // ---- Meetings --------------------------------------------------------
    // POST /calling/meetings
    void createMeeting(
        const std::string& title,
        const std::string& password,
        int max_participants,
        AnyChatValueCallback<MeetingRoom> callback
    );

    // POST /calling/meetings/{roomId}/join
    void joinMeeting(const std::string& room_id, const std::string& password, AnyChatValueCallback<MeetingRoom> callback);

    // POST /calling/meetings/{roomId}/end
    void endMeeting(const std::string& room_id, AnyChatCallback callback);

    // GET /calling/meetings/{roomId}
    void getMeeting(const std::string& room_id, AnyChatValueCallback<MeetingRoom> callback);

    // GET /calling/meetings?page=&pageSize=
    void listMeetings(int page, int page_size, AnyChatValueCallback<MeetingListResult> callback);

    // ---- WebSocket notification handlers ---------------------------------
    // livekit.call_invite / livekit.call_status / livekit.call_rejected.
    void setListener(std::shared_ptr<CallListener> listener);

private:
    void handleCallNotification(const NotificationEvent& event);

    std::shared_ptr<network::HttpClient> http_;

    std::mutex handler_mutex_;
    std::shared_ptr<CallListener> listener_;
};

} // namespace anychat
