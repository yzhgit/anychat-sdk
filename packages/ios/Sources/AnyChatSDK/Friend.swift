//
//  Friend.swift
//  AnyChatSDK
//
//  Friend manager with async/await support
//

import Foundation

public actor FriendManager {
    private let handle: AnyChatFriendHandle
    private var requestReceivedContinuation: AsyncStream<FriendRequest>.Continuation?
    private var listChangedContinuation: AsyncStream<Void>.Continuation?

    init(handle: AnyChatFriendHandle) {
        self.handle = handle
        setupCallbacks()
    }

    // MARK: - Friend Operations

    public func getList() async throws -> [Friend] {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendListCallback = { userdata, list, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<[Friend]>>.fromOpaque(userdata).takeRetainedValue()

                if let list = list {
                    let friends = convertFriendList(list)
                    context.continuation.resume(returning: friends)
                    var mutableList = list.pointee
                    mutableList.free()
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to fetch friends"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            let result = anychat_friend_get_list(handle, userdata, callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[Friend]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    public func sendRequest(toUserId: String, message: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Send request failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(toUserId) { userIdPtr in
                withCString(message) { msgPtr in
                    let result = anychat_friend_send_request(handle, userIdPtr, msgPtr, userdata, callback)
                    if result != ANYCHAT_OK {
                        let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                        ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                    }
                }
            }
        }
    }

    public func handleRequest(requestId: Int64, accept: Bool) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Handle request failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            let result = anychat_friend_handle_request(
                handle,
                requestId,
                accept ? 1 : 0,
                userdata,
                callback
            )

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    public func getPendingRequests() async throws -> [FriendRequest] {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendRequestListCallback = { userdata, list, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<[FriendRequest]>>.fromOpaque(userdata).takeRetainedValue()

                if let list = list {
                    let requests = convertFriendRequestList(list)
                    context.continuation.resume(returning: requests)
                    var mutableList = list.pointee
                    mutableList.free()
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to fetch requests"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            let result = anychat_friend_get_pending_requests(handle, userdata, callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[FriendRequest]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    public func delete(friendId: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Delete failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(friendId) { friendPtr in
                let result = anychat_friend_delete(handle, friendPtr, userdata, callback)
                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    public func updateRemark(friendId: String, remark: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Update remark failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(friendId) { friendPtr in
                withCString(remark) { remarkPtr in
                    let result = anychat_friend_update_remark(handle, friendPtr, remarkPtr, userdata, callback)
                    if result != ANYCHAT_OK {
                        let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                        ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                    }
                }
            }
        }
    }

    public func addToBlacklist(userId: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Add to blacklist failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(userId) { userPtr in
                let result = anychat_friend_add_to_blacklist(handle, userPtr, userdata, callback)
                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    public func removeFromBlacklist(userId: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatFriendCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Remove from blacklist failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(userId) { userPtr in
                let result = anychat_friend_remove_from_blacklist(handle, userPtr, userdata, callback)
                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    // MARK: - Event Streams

    public var requestReceived: AsyncStream<FriendRequest> {
        AsyncStream { continuation in
            self.requestReceivedContinuation = continuation
        }
    }

    public var listChanged: AsyncStream<Void> {
        AsyncStream { continuation in
            self.listChangedContinuation = continuation
        }
    }

    // MARK: - Private

    private func setupCallbacks() {
        let requestCallback: AnyChatFriendRequestCallback = { userdata, request in
            guard let userdata = userdata, let request = request else { return }
            let context = Unmanaged<StreamContext<FriendRequest>>.fromOpaque(userdata).takeUnretainedValue()
            context.continuation.yield(FriendRequest(from: request.pointee))
        }

        let changedCallback: AnyChatFriendListChangedCallback = { userdata in
            guard let userdata = userdata else { return }
            let context = Unmanaged<StreamContext<Void>>.fromOpaque(userdata).takeUnretainedValue()
            context.continuation.yield(())
        }

        Task {
            if let cont = requestReceivedContinuation {
                let context = StreamContext(continuation: cont)
                let userdata = Unmanaged.passRetained(context).toOpaque()
                anychat_friend_set_request_callback(handle, userdata, requestCallback)
            }

            if let cont = listChangedContinuation {
                let context = StreamContext(continuation: cont)
                let userdata = Unmanaged.passRetained(context).toOpaque()
                anychat_friend_set_list_changed_callback(handle, userdata, changedCallback)
            }
        }
    }
}
