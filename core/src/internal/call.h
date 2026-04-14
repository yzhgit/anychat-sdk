#pragma once

#include "callbacks.h"
#include "types.h"

#include <memory>
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

class CallManager {
public:
    virtual ~CallManager() = default;

    // ---- One-to-one calls ------------------------------------------------

    // POST /calling/calls
    virtual void initiateCall(
        const std::string& callee_id,
        CallType type,
        AnyChatValueCallback<CallSession> callback
    ) = 0;

    // POST /calling/calls/{callId}/join
    virtual void joinCall(const std::string& call_id, AnyChatValueCallback<CallSession> callback) = 0;

    // POST /calling/calls/{callId}/reject
    virtual void rejectCall(const std::string& call_id, AnyChatCallback callback) = 0;

    // POST /calling/calls/{callId}/end
    virtual void endCall(const std::string& call_id, AnyChatCallback callback) = 0;

    // GET  /calling/calls/{callId}
    virtual void getCallSession(const std::string& call_id, AnyChatValueCallback<CallSession> callback) = 0;

    // GET  /calling/calls?page=&pageSize=
    virtual void getCallLogs(int page, int page_size, AnyChatValueCallback<CallLogResult> callback) = 0;

    // ---- Meetings --------------------------------------------------------

    // POST /calling/meetings
    virtual void createMeeting(
        const std::string& title,
        const std::string& password,
        int max_participants,
        AnyChatValueCallback<MeetingRoom> callback
    ) = 0;

    // POST /calling/meetings/{roomId}/join
    virtual void
    joinMeeting(const std::string& room_id, const std::string& password, AnyChatValueCallback<MeetingRoom> callback) = 0;

    // POST /calling/meetings/{roomId}/end
    virtual void endMeeting(const std::string& room_id, AnyChatCallback callback) = 0;

    // GET  /calling/meetings/{roomId}
    virtual void getMeeting(const std::string& room_id, AnyChatValueCallback<MeetingRoom> callback) = 0;

    // GET  /calling/meetings?page=&pageSize=
    virtual void listMeetings(int page, int page_size, AnyChatValueCallback<MeetingListResult> callback) = 0;

    // ---- WebSocket notification handlers ---------------------------------

    // livekit.call_invite / livekit.call_status / livekit.call_rejected.
    virtual void setListener(std::shared_ptr<CallListener> listener) = 0;
};

} // namespace anychat
