#include "call_manager.h"

#include "json_common.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace anychat {
namespace call_detail {

using json_common::ApiEnvelope;
using json_common::parseApiEnvelopeResponse;
using json_common::parseBoolValue;
using json_common::parseInt64Value;
using json_common::parseJsonObject;
using json_common::toLower;
using json_common::writeJson;

using IntegerValue = std::variant<int64_t, double, std::string>;
using OptionalIntegerValue = std::optional<IntegerValue>;
using BooleanValue = std::variant<bool, int64_t, double, std::string>;
using OptionalBooleanValue = std::optional<BooleanValue>;
using StatusValue = std::variant<int64_t, double, std::string>;
using OptionalStatusValue = std::optional<StatusValue>;
using CallTypeValue = std::variant<int64_t, double, std::string>;
using OptionalCallTypeValue = std::optional<CallTypeValue>;

struct InitiateCallRequest {
    std::string callee_id{};
    std::string call_type{};
};

struct CreateMeetingRequest {
    std::string title{};
    std::optional<std::string> password{};
    std::optional<int32_t> max_participants{};
};

struct JoinMeetingRequest {
    std::optional<std::string> password{};
};

struct CallSessionPayload {
    std::string call_id{};
    std::string caller_id{};
    std::string callee_id{};
    std::string room_name{};
    std::string token{};
    OptionalIntegerValue started_at{};
    OptionalIntegerValue connected_at{};
    OptionalIntegerValue ended_at{};
    OptionalIntegerValue duration{};
    OptionalCallTypeValue call_type{};
    OptionalStatusValue status{};
};

struct CallSessionListDataPayload {
    OptionalIntegerValue total{};
    std::optional<std::vector<CallSessionPayload>> sessions{};
};

struct MeetingRoomPayload {
    std::string room_id{};
    std::string creator_id{};
    std::string title{};
    std::string room_name{};
    std::string token{};
    OptionalBooleanValue has_password{};
    OptionalIntegerValue max_participants{};
    OptionalIntegerValue started_at{};
    OptionalIntegerValue created_at{};
    OptionalStatusValue status{};
};

struct MeetingResultDataPayload {
    std::optional<MeetingRoomPayload> meeting{};
    std::string token{};
};

struct MeetingListDataPayload {
    OptionalIntegerValue total{};
    std::optional<std::vector<MeetingRoomPayload>> meetings{};
};

struct CallStatusNotificationPayload {
    std::string call_id{};
    OptionalStatusValue status{};
};

CallType parseCallTypeValue(const OptionalCallTypeValue& value) {
    if (!value.has_value()) {
        return CallType::Audio;
    }

    if (const auto* int_value = std::get_if<int64_t>(&*value); int_value != nullptr) {
        return *int_value == 1 ? CallType::Video : CallType::Audio;
    }
    if (const auto* dbl_value = std::get_if<double>(&*value); dbl_value != nullptr) {
        return static_cast<int64_t>(*dbl_value) == 1 ? CallType::Video : CallType::Audio;
    }

    return toLower(std::get<std::string>(*value)) == "video" ? CallType::Video : CallType::Audio;
}

CallStatus parseCallStatusValue(const OptionalStatusValue& value, CallStatus def = CallStatus::Ringing) {
    if (!value.has_value()) {
        return def;
    }

    if (const auto* int_value = std::get_if<int64_t>(&*value); int_value != nullptr) {
        switch (*int_value) {
        case 1:
            return CallStatus::Connected;
        case 2:
            return CallStatus::Ended;
        case 3:
            return CallStatus::Rejected;
        case 4:
            return CallStatus::Missed;
        case 5:
            return CallStatus::Cancelled;
        default:
            return def;
        }
    }
    if (const auto* dbl_value = std::get_if<double>(&*value); dbl_value != nullptr) {
        return parseCallStatusValue(OptionalStatusValue{ static_cast<int64_t>(*dbl_value) }, def);
    }

    const std::string status = toLower(std::get<std::string>(*value));
    if (status == "connected")
        return CallStatus::Connected;
    if (status == "ended")
        return CallStatus::Ended;
    if (status == "rejected")
        return CallStatus::Rejected;
    if (status == "missed")
        return CallStatus::Missed;
    if (status == "cancelled")
        return CallStatus::Cancelled;
    return def;
}

bool parseMeetingIsActive(const OptionalStatusValue& value) {
    if (!value.has_value()) {
        return true;
    }

    if (const auto* int_value = std::get_if<int64_t>(&*value); int_value != nullptr) {
        return *int_value != 1;
    }
    if (const auto* dbl_value = std::get_if<double>(&*value); dbl_value != nullptr) {
        return static_cast<int64_t>(*dbl_value) != 1;
    }
    return toLower(std::get<std::string>(*value)) != "ended";
}

CallSession parseCallSessionPayload(const CallSessionPayload& payload) {
    CallSession session;
    session.call_id = payload.call_id;
    session.caller_id = payload.caller_id;
    session.callee_id = payload.callee_id;
    session.room_name = payload.room_name;
    session.token = payload.token;
    session.started_at = parseInt64Value(payload.started_at, 0);
    session.connected_at = parseInt64Value(payload.connected_at, 0);
    session.ended_at = parseInt64Value(payload.ended_at, 0);
    session.duration = static_cast<int32_t>(parseInt64Value(payload.duration, 0));
    session.call_type = parseCallTypeValue(payload.call_type);
    session.status = parseCallStatusValue(payload.status, CallStatus::Ringing);
    return session;
}

MeetingRoom parseMeetingRoomPayload(const MeetingRoomPayload& payload) {
    MeetingRoom room;
    room.room_id = payload.room_id;
    room.creator_id = payload.creator_id;
    room.title = payload.title;
    room.room_name = payload.room_name;
    room.token = payload.token;
    room.has_password = parseBoolValue(payload.has_password, false);
    room.max_participants = static_cast<int32_t>(parseInt64Value(payload.max_participants, 0));
    room.started_at = parseInt64Value(payload.started_at, 0);
    room.created_at_ms = parseInt64Value(payload.created_at, 0) * 1000;
    room.is_active = parseMeetingIsActive(payload.status);
    return room;
}

MeetingRoom parseMeetingResult(const MeetingResultDataPayload& wrapped) {
    MeetingRoom room;
    if (wrapped.meeting.has_value()) {
        room = parseMeetingRoomPayload(*wrapped.meeting);
    }
    if (!wrapped.token.empty()) {
        room.token = wrapped.token;
    }
    return room;
}

const std::vector<CallSessionPayload>* toCallSessionPayloadList(const CallSessionListDataPayload& data) {
    return data.sessions.has_value() ? &(*data.sessions) : nullptr;
}

const std::vector<MeetingRoomPayload>* toMeetingRoomPayloadList(const MeetingListDataPayload& data) {
    return data.meetings.has_value() ? &(*data.meetings) : nullptr;
}

} // namespace call_detail

using namespace call_detail;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

CallManagerImpl::CallManagerImpl(std::shared_ptr<network::HttpClient> http, NotificationManager* notif_mgr)
    : http_(std::move(http)) {
    if (notif_mgr) {
        notif_mgr->addNotificationHandler([this](const NotificationEvent& ev) {
            handleCallNotification(ev);
        });
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void CallManagerImpl::initiateCall(
    const std::string& callee_id,
    CallType type,
    AnyChatValueCallback<CallSession> callback
) {
    InitiateCallRequest body{};
    body.callee_id = callee_id;
    body.call_type = (type == CallType::Video) ? "video" : "audio";

    std::string body_json;
    std::string err;
    if (!writeJson(body, body_json, err)) {
        if (callback.on_error) {
            callback.on_error(-1, err);
        }
        return;
    }

    http_->post("/calling/calls", body_json, [cb = std::move(callback)](network::HttpResponse resp) {
        ApiEnvelope<CallSessionPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root, "initiate call failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }
        if (cb.on_success) {
            cb.on_success(parseCallSessionPayload(root.data));
        }
    });
}

void CallManagerImpl::joinCall(const std::string& call_id, AnyChatValueCallback<CallSession> callback) {
    http_->post(
        "/calling/calls/" + call_id + "/join",
        "",
        [cb = std::move(callback), call_id](network::HttpResponse resp) {
            ApiEnvelope<CallSessionPayload> root{};
            if (!parseApiEnvelopeResponse(resp, root, "join call failed", true)) {
                if (cb.on_error) {
                    cb.on_error(root.code, root.message);
                }
                return;
            }

            CallSession session = parseCallSessionPayload(root.data);
            if (session.call_id.empty()) {
                session.call_id = call_id;
            }
            if (cb.on_success) {
                cb.on_success(session);
            }
        }
    );
}

void CallManagerImpl::rejectCall(const std::string& call_id, AnyChatCallback callback) {
    http_->post("/calling/calls/" + call_id + "/reject", "", [cb = std::move(callback)](network::HttpResponse resp) {
        ApiEnvelope<void> root{};
        if (!parseApiEnvelopeResponse(resp, root, "reject call failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }
        if (cb.on_success) {
            cb.on_success();
        }
    });
}

void CallManagerImpl::endCall(const std::string& call_id, AnyChatCallback callback) {
    http_->post("/calling/calls/" + call_id + "/end", "", [cb = std::move(callback)](network::HttpResponse resp) {
        ApiEnvelope<void> root{};
        if (!parseApiEnvelopeResponse(resp, root, "end call failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }
        if (cb.on_success) {
            cb.on_success();
        }
    });
}

void CallManagerImpl::getCallSession(const std::string& call_id, AnyChatValueCallback<CallSession> callback) {
    http_->get("/calling/calls/" + call_id, [cb = std::move(callback), call_id](network::HttpResponse resp) {
        ApiEnvelope<CallSessionPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root, "get call session failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }
        CallSession session = parseCallSessionPayload(root.data);
        if (session.call_id.empty()) {
            session.call_id = call_id;
        }
        if (cb.on_success) {
            cb.on_success(session);
        }
    });
}

void CallManagerImpl::getCallLogs(int page, int page_size, AnyChatValueCallback<CallLogResult> callback) {
    std::string path = "/calling/calls?page=" + std::to_string(page) + "&page_size=" + std::to_string(page_size);

    http_->get(path, [cb = std::move(callback)](network::HttpResponse resp) {
        ApiEnvelope<CallSessionListDataPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root, "get call logs failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        CallLogResult result;
        result.total = parseInt64Value(root.data.total, 0);

        const auto* payloads = toCallSessionPayloadList(root.data);
        if (payloads != nullptr) {
            result.calls.reserve(payloads->size());
            for (const auto& item : *payloads) {
                result.calls.push_back(parseCallSessionPayload(item));
            }
        }
        if (cb.on_success) {
            cb.on_success(result);
        }
    });
}

void CallManagerImpl::createMeeting(
    const std::string& title,
    const std::string& password,
    int max_participants,
    AnyChatValueCallback<MeetingRoom> callback
) {
    CreateMeetingRequest body{};
    body.title = title;
    if (!password.empty()) {
        body.password = password;
    }
    if (max_participants > 0) {
        body.max_participants = max_participants;
    }

    std::string body_json;
    std::string err;
    if (!writeJson(body, body_json, err)) {
        if (callback.on_error) {
            callback.on_error(-1, err);
        }
        return;
    }

    http_->post("/calling/meetings", body_json, [cb = std::move(callback)](network::HttpResponse resp) {
        ApiEnvelope<MeetingResultDataPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root, "create meeting failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        if (cb.on_success) {
            cb.on_success(parseMeetingResult(root.data));
        }
    });
}

void CallManagerImpl::joinMeeting(
    const std::string& room_id,
    const std::string& password,
    AnyChatValueCallback<MeetingRoom> callback
) {
    JoinMeetingRequest body{};
    if (!password.empty()) {
        body.password = password;
    }

    std::string body_json;
    std::string err;
    if (!writeJson(body, body_json, err)) {
        if (callback.on_error) {
            callback.on_error(-1, err);
        }
        return;
    }

    http_->post(
        "/calling/meetings/" + room_id + "/join",
        body_json,
        [cb = std::move(callback), room_id](network::HttpResponse resp) {
            ApiEnvelope<MeetingResultDataPayload> root{};
            if (!parseApiEnvelopeResponse(resp, root, "join meeting failed", true)) {
                if (cb.on_error) {
                    cb.on_error(root.code, root.message);
                }
                return;
            }

            MeetingRoom room = parseMeetingResult(root.data);
            if (room.room_id.empty()) {
                room.room_id = room_id;
            }
            if (cb.on_success) {
                cb.on_success(room);
            }
        }
    );
}

void CallManagerImpl::endMeeting(const std::string& room_id, AnyChatCallback callback) {
    http_->post("/calling/meetings/" + room_id + "/end", "", [cb = std::move(callback)](network::HttpResponse resp) {
        ApiEnvelope<void> root{};
        if (!parseApiEnvelopeResponse(resp, root, "end meeting failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }
        if (cb.on_success) {
            cb.on_success();
        }
    });
}

void CallManagerImpl::getMeeting(const std::string& room_id, AnyChatValueCallback<MeetingRoom> callback) {
    http_->get("/calling/meetings/" + room_id, [cb = std::move(callback), room_id](network::HttpResponse resp) {
        ApiEnvelope<MeetingRoomPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root, "get meeting failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }
        MeetingRoom room = parseMeetingRoomPayload(root.data);
        if (room.room_id.empty()) {
            room.room_id = room_id;
        }
        if (cb.on_success) {
            cb.on_success(room);
        }
    });
}

void CallManagerImpl::listMeetings(int page, int page_size, AnyChatValueCallback<MeetingListResult> callback) {
    std::string path = "/calling/meetings?page=" + std::to_string(page) + "&page_size=" + std::to_string(page_size);

    http_->get(path, [cb = std::move(callback)](network::HttpResponse resp) {
        ApiEnvelope<MeetingListDataPayload> root{};
        if (!parseApiEnvelopeResponse(resp, root, "list meetings failed", true)) {
            if (cb.on_error) {
                cb.on_error(root.code, root.message);
            }
            return;
        }

        MeetingListResult result;
        result.total = parseInt64Value(root.data.total, 0);

        const auto* payloads = toMeetingRoomPayloadList(root.data);
        if (payloads != nullptr) {
            result.rooms.reserve(payloads->size());
            for (const auto& item : *payloads) {
                result.rooms.push_back(parseMeetingRoomPayload(item));
            }
        }
        if (cb.on_success) {
            cb.on_success(result);
        }
    });
}

// ---------------------------------------------------------------------------
// Notification handlers
// ---------------------------------------------------------------------------

void CallManagerImpl::setListener(std::shared_ptr<CallListener> listener) {
    std::lock_guard<std::mutex> lk(handler_mutex_);
    listener_ = std::move(listener);
}

void CallManagerImpl::handleCallNotification(const NotificationEvent& event) {
    const auto& nt = event.notification_type;

    if (nt == "livekit.call_invite") {
        std::shared_ptr<CallListener> listener;
        {
            std::lock_guard<std::mutex> lk(handler_mutex_);
            listener = listener_;
        }
        if (listener) {
            try {
                CallSessionPayload payload{};
                std::string err;
                if (!parseJsonObject(event.data, payload, err)) {
                    return;
                }
                listener->onIncomingCall(parseCallSessionPayload(payload));
            } catch (...) {
            }
        }
        return;
    }

    if (nt == "livekit.call_status" || nt == "livekit.call_rejected") {
        std::shared_ptr<CallListener> listener;
        {
            std::lock_guard<std::mutex> lk(handler_mutex_);
            listener = listener_;
        }
        if (listener) {
            try {
                CallStatusNotificationPayload payload{};
                std::string err;
                if (!parseJsonObject(event.data, payload, err)) {
                    return;
                }

                const CallStatus status =
                    (nt == "livekit.call_rejected")
                        ? CallStatus::Rejected
                        : parseCallStatusValue(payload.status, CallStatus::Ringing);
                listener->onCallStatusChanged(payload.call_id, status);
            } catch (...) {
            }
        }
        return;
    }
}

} // namespace anychat
