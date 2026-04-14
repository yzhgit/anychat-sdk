//
//  Call.swift
//  AnyChatSDK
//
//  Call (Voice/Video) manager with async/await support
//

import Foundation

private final class CallListenerContext: @unchecked Sendable {
    var incomingCallContinuation: AsyncStream<CallSession>.Continuation?
    var callStatusChangedContinuation: AsyncStream<(String, CallStatus)>.Continuation?
}

public actor CallManager {
    private let handle: AnyChatCallHandle
    private let listenerContext = CallListenerContext()
    private var listenerUserdata: UnsafeMutableRawPointer?

    init(handle: AnyChatCallHandle) {
        self.handle = handle
    }

    deinit {
        anychat_call_set_listener(handle, nil)
        if let userdata = listenerUserdata {
            Unmanaged<CallListenerContext>.fromOpaque(userdata).release()
            listenerUserdata = nil
        }
    }

    // MARK: - One-to-One Call Operations

    public func initiateCall(
        calleeId: String,
        callType: CallType
    ) async throws -> CallSession {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<CallSession, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatCallSessionCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatCallSessionCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, session in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let session else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: CallSession(from: session.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(calleeId) { calleePtr in
                anychat_call_initiate_call(
                    handle,
                    calleePtr,
                    Int32(callType.rawValue),
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func joinCall(callId: String) async throws -> CallSession {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<CallSession, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatCallSessionCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatCallSessionCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, session in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let session else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: CallSession(from: session.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(callId) { callIdPtr in
                anychat_call_join_call(handle, callIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func rejectCall(callId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatCallCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatCallCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                ctx.continuation.resume(returning: ())
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(callId) { callIdPtr in
                anychat_call_reject_call(handle, callIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func endCall(callId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatCallCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatCallCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                ctx.continuation.resume(returning: ())
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(callId) { callIdPtr in
                anychat_call_end_call(handle, callIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func getCallSession(callId: String) async throws -> CallSession {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<CallSession, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatCallSessionCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatCallSessionCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, session in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let session else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: CallSession(from: session.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(callId) { callIdPtr in
                anychat_call_get_call_session(handle, callIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<CallSession>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func getCallLogs(
        page: Int = 1,
        pageSize: Int = 20
    ) async throws -> ([CallSession], Int64) {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<([CallSession], Int64), Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatCallListCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatCallListCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<([CallSession], Int64)>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: ([], 0))
                    return
                }
                let result = convertCallList(list)
                ctx.continuation.resume(returning: result)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<([CallSession], Int64)>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_call_get_call_logs(
                handle,
                Int32(page),
                Int32(pageSize),
                &callback
            )

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<([CallSession], Int64)>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Meeting Operations

    public func createMeeting(
        title: String,
        password: String? = nil,
        maxParticipants: Int = 50
    ) async throws -> MeetingRoom {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<MeetingRoom, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatMeetingCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatMeetingCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, room in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let room else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: MeetingRoom(from: room.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(title) { titlePtr in
                withOptionalCString(password) { passwordPtr in
                    anychat_call_create_meeting(
                        handle,
                        titlePtr,
                        passwordPtr,
                        Int32(maxParticipants),
                        &callback
                    )
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func joinMeeting(
        roomId: String,
        password: String? = nil
    ) async throws -> MeetingRoom {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<MeetingRoom, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatMeetingCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatMeetingCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, room in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let room else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: MeetingRoom(from: room.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(roomId) { roomIdPtr in
                withOptionalCString(password) { passwordPtr in
                    anychat_call_join_meeting(
                        handle,
                        roomIdPtr,
                        passwordPtr,
                        &callback
                    )
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func endMeeting(roomId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatCallCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatCallCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                ctx.continuation.resume(returning: ())
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(roomId) { roomIdPtr in
                anychat_call_end_meeting(handle, roomIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func getMeeting(roomId: String) async throws -> MeetingRoom {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<MeetingRoom, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatMeetingCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatMeetingCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, room in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let room else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: MeetingRoom(from: room.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(roomId) { roomIdPtr in
                anychat_call_get_meeting(handle, roomIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<MeetingRoom>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func listMeetings(
        page: Int = 1,
        pageSize: Int = 20
    ) async throws -> ([MeetingRoom], Int64) {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<([MeetingRoom], Int64), Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatMeetingListCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatMeetingListCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<([MeetingRoom], Int64)>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: ([], 0))
                    return
                }
                let result = convertMeetingList(list)
                ctx.continuation.resume(returning: result)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<([MeetingRoom], Int64)>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_call_list_meetings(
                handle,
                Int32(page),
                Int32(pageSize),
                &callback
            )

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<([MeetingRoom], Int64)>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Event Streams

    public var incomingCall: AsyncStream<CallSession> {
        AsyncStream { continuation in
            Task { await self.setIncomingCallContinuation(continuation) }
        }
    }

    public var callStatusChanged: AsyncStream<(String, CallStatus)> {
        AsyncStream { continuation in
            Task { await self.setCallStatusContinuation(continuation) }
        }
    }

    // MARK: - Listener

    private func setIncomingCallContinuation(_ continuation: AsyncStream<CallSession>.Continuation) {
        listenerContext.incomingCallContinuation = continuation
        refreshListener()
    }

    private func setCallStatusContinuation(_ continuation: AsyncStream<(String, CallStatus)>.Continuation) {
        listenerContext.callStatusChangedContinuation = continuation
        refreshListener()
    }

    private func refreshListener() {
        let hasIncoming = listenerContext.incomingCallContinuation != nil
        let hasStatus = listenerContext.callStatusChangedContinuation != nil

        guard hasIncoming || hasStatus else {
            anychat_call_set_listener(handle, nil)
            if let userdata = listenerUserdata {
                Unmanaged<CallListenerContext>.fromOpaque(userdata).release()
                listenerUserdata = nil
            }
            return
        }

        if listenerUserdata == nil {
            listenerUserdata = Unmanaged.passRetained(listenerContext).toOpaque()
        }

        var listener = AnyChatCallListener_C()
        listener.struct_size = UInt32(MemoryLayout<AnyChatCallListener_C>.size)
        listener.userdata = listenerUserdata

        if hasIncoming {
            listener.on_incoming_call = { userdata, session in
                guard let userdata, let session else { return }
                let context = Unmanaged<CallListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.incomingCallContinuation?.yield(CallSession(from: session.pointee))
            }
        }

        if hasStatus {
            listener.on_call_status_changed = { userdata, callId, status in
                guard let userdata, let callId else { return }
                let context = Unmanaged<CallListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                guard let mapped = CallStatus(rawValue: Int(status)) else { return }
                context.callStatusChangedContinuation?.yield((String(cString: callId), mapped))
            }
        }

        anychat_call_set_listener(handle, &listener)
    }
}
