//
//  Friend.swift
//  AnyChatSDK
//
//  Friend manager with async/await support
//

import Foundation

private final class FriendListenerContext: @unchecked Sendable {
    var requestContinuation: AsyncStream<FriendRequest>.Continuation?
    var listChangedContinuation: AsyncStream<Void>.Continuation?
}

public actor FriendManager {
    private let handle: AnyChatFriendHandle
    private let listenerContext = FriendListenerContext()
    private var listenerUserdata: UnsafeMutableRawPointer?

    init(handle: AnyChatFriendHandle) {
        self.handle = handle
    }

    deinit {
        anychat_friend_set_listener(handle, nil)
        if let userdata = listenerUserdata {
            Unmanaged<FriendListenerContext>.fromOpaque(userdata).release()
            listenerUserdata = nil
        }
    }

    // MARK: - Friend Operations

    public func getFriendList() async throws -> [Friend] {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<[Friend], Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendListCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendListCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Friend]>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: [])
                    return
                }
                let friends = convertFriendList(list)
                ctx.continuation.resume(returning: friends)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Friend]>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_friend_get_list(handle, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[Friend]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func addFriend(
        toUserId: String,
        message: String,
        source: String = "ios"
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendCallback_C>.size)
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

            let result = withCString(toUserId) { userIdPtr in
                withCString(message) { messagePtr in
                    withCString(source) { sourcePtr in
                        anychat_friend_add(
                            handle,
                            userIdPtr,
                            messagePtr,
                            sourcePtr,
                            &callback
                        )
                    }
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func handleFriendRequest(requestId: Int64, accept: Bool) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendCallback_C>.size)
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

            let result = accept
                ? anychat_friend_accept_request(handle, requestId, &callback)
                : anychat_friend_reject_request(handle, requestId, &callback)

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func getPendingRequests(type: String = "received") async throws -> [FriendRequest] {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<[FriendRequest], Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendRequestListCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendRequestListCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[FriendRequest]>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: [])
                    return
                }
                let requests = convertFriendRequestList(list)
                ctx.continuation.resume(returning: requests)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[FriendRequest]>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(type) { typePtr in
                anychat_friend_get_requests(handle, typePtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[FriendRequest]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func delete(friendId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendCallback_C>.size)
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

            let result = withCString(friendId) { friendPtr in
                anychat_friend_delete(handle, friendPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func updateRemark(friendId: String, remark: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendCallback_C>.size)
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

            let result = withCString(friendId) { friendPtr in
                withCString(remark) { remarkPtr in
                    anychat_friend_update_remark(handle, friendPtr, remarkPtr, &callback)
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func addToBlacklist(userId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendCallback_C>.size)
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

            let result = withCString(userId) { userPtr in
                anychat_friend_add_to_blacklist(handle, userPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func removeFromBlacklist(userId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatFriendCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatFriendCallback_C>.size)
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

            let result = withCString(userId) { userPtr in
                anychat_friend_remove_from_blacklist(handle, userPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Event Streams

    public var requestReceived: AsyncStream<FriendRequest> {
        AsyncStream { continuation in
            Task { await self.setRequestContinuation(continuation) }
        }
    }

    public var listChanged: AsyncStream<Void> {
        AsyncStream { continuation in
            Task { await self.setListChangedContinuation(continuation) }
        }
    }

    // MARK: - Listener

    private func setRequestContinuation(_ continuation: AsyncStream<FriendRequest>.Continuation) {
        listenerContext.requestContinuation = continuation
        refreshListener()
    }

    private func setListChangedContinuation(_ continuation: AsyncStream<Void>.Continuation) {
        listenerContext.listChangedContinuation = continuation
        refreshListener()
    }

    private func refreshListener() {
        let hasRequest = listenerContext.requestContinuation != nil
        let hasListChanged = listenerContext.listChangedContinuation != nil

        guard hasRequest || hasListChanged else {
            anychat_friend_set_listener(handle, nil)
            if let userdata = listenerUserdata {
                Unmanaged<FriendListenerContext>.fromOpaque(userdata).release()
                listenerUserdata = nil
            }
            return
        }

        if listenerUserdata == nil {
            listenerUserdata = Unmanaged.passRetained(listenerContext).toOpaque()
        }

        var listener = AnyChatFriendListener_C()
        listener.struct_size = UInt32(MemoryLayout<AnyChatFriendListener_C>.size)
        listener.userdata = listenerUserdata

        if hasRequest {
            listener.on_friend_request_received = { userdata, request in
                guard let userdata, let request else { return }
                let context = Unmanaged<FriendListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.requestContinuation?.yield(FriendRequest(from: request.pointee))
            }
        }

        if hasListChanged {
            listener.on_friend_added = { userdata, _ in
                guard let userdata else { return }
                let context = Unmanaged<FriendListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.listChangedContinuation?.yield(())
            }
            listener.on_friend_deleted = { userdata, _ in
                guard let userdata else { return }
                let context = Unmanaged<FriendListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.listChangedContinuation?.yield(())
            }
            listener.on_friend_info_changed = { userdata, _ in
                guard let userdata else { return }
                let context = Unmanaged<FriendListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.listChangedContinuation?.yield(())
            }
            listener.on_friend_request_deleted = { userdata, _ in
                guard let userdata else { return }
                let context = Unmanaged<FriendListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.listChangedContinuation?.yield(())
            }
            listener.on_friend_request_accepted = { userdata, _ in
                guard let userdata else { return }
                let context = Unmanaged<FriendListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.listChangedContinuation?.yield(())
            }
            listener.on_friend_request_rejected = { userdata, _ in
                guard let userdata else { return }
                let context = Unmanaged<FriendListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.listChangedContinuation?.yield(())
            }
        }

        anychat_friend_set_listener(handle, &listener)
    }
}
