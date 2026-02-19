//
//  Message.swift
//  AnyChatSDK
//
//  Message manager with async/await support
//

import Foundation

public actor MessageManager {
    private let handle: AnyChatMessageHandle
    private var receivedContinuation: AsyncStream<Message>.Continuation?

    init(handle: AnyChatMessageHandle) {
        self.handle = handle
        setupCallbacks()
    }

    // MARK: - Message Operations

    public func sendText(
        sessionId: String,
        content: String
    ) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatMessageCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Send failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(sessionId) { sessionPtr in
                withCString(content) { contentPtr in
                    let result = anychat_message_send_text(
                        handle,
                        sessionPtr,
                        contentPtr,
                        userdata,
                        callback
                    )

                    if result != ANYCHAT_OK {
                        let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                        ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                    }
                }
            }
        }
    }

    public func getHistory(
        sessionId: String,
        beforeTimestamp: Date? = nil,
        limit: Int = 20
    ) async throws -> [Message] {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatMessageListCallback = { userdata, list, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<[Message]>>.fromOpaque(userdata).takeRetainedValue()

                if let list = list {
                    let messages = convertMessageList(list)
                    context.continuation.resume(returning: messages)
                    var mutableList = list.pointee
                    mutableList.free()
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to fetch history"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            let timestampMs = beforeTimestamp.map { Int64($0.timeIntervalSince1970 * 1000) } ?? 0

            withCString(sessionId) { sessionPtr in
                let result = anychat_message_get_history(
                    handle,
                    sessionPtr,
                    timestampMs,
                    Int32(limit),
                    userdata,
                    callback
                )

                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<[Message]>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    public func markRead(
        sessionId: String,
        messageId: String
    ) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatMessageCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Mark read failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(sessionId) { sessionPtr in
                withCString(messageId) { messagePtr in
                    let result = anychat_message_mark_read(
                        handle,
                        sessionPtr,
                        messagePtr,
                        userdata,
                        callback
                    )

                    if result != ANYCHAT_OK {
                        let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                        ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                    }
                }
            }
        }
    }

    // MARK: - Event Streams

    public var receivedMessages: AsyncStream<Message> {
        AsyncStream { continuation in
            self.receivedContinuation = continuation
        }
    }

    // MARK: - Private

    private func setupCallbacks() {
        let callback: AnyChatMessageReceivedCallback = { userdata, message in
            guard let userdata = userdata, let message = message else { return }
            let context = Unmanaged<StreamContext<Message>>.fromOpaque(userdata).takeUnretainedValue()
            context.continuation.yield(Message(from: message.pointee))
        }

        Task {
            let context = StreamContext(continuation: receivedContinuation!)
            let userdata = Unmanaged.passRetained(context).toOpaque()
            anychat_message_set_received_callback(handle, userdata, callback)
        }
    }
}
