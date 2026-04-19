//
//  Message.swift
//  AnyChatSDK
//
//  Message manager with async/await support
//

import Foundation

private final class MessageListenerContext: @unchecked Sendable {
    var continuation: AsyncStream<Message>.Continuation?
}

public actor MessageManager {
    private let handle: AnyChatMessageHandle
    private let listenerContext = MessageListenerContext()
    private var listenerUserdata: UnsafeMutableRawPointer?

    init(handle: AnyChatMessageHandle) {
        self.handle = handle
    }

    deinit {
        anychat_message_set_listener(handle, nil)
        if let userdata = listenerUserdata {
            Unmanaged<MessageListenerContext>.fromOpaque(userdata).release()
            listenerUserdata = nil
        }
    }

    // MARK: - Message Operations

    public func sendText(
        sessionId: String,
        content: String
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatMessageCallback_C()
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

            let result = withCString(sessionId) { sessionPtr in
                withCString(content) { contentPtr in
                    anychat_message_send_text(
                        handle,
                        sessionPtr,
                        contentPtr,
                        &callback
                    )
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func getHistory(
        sessionId: String,
        beforeTimestamp: Date? = nil,
        limit: Int = 20
    ) async throws -> [Message] {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<[Message], Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatMessageListCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Message]>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: [])
                    return
                }
                let messages = convertMessageList(list)
                ctx.continuation.resume(returning: messages)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Message]>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let timestampMs = beforeTimestamp.map { Int64($0.timeIntervalSince1970 * 1000) } ?? 0
            let result = withCString(sessionId) { sessionPtr in
                anychat_message_get_history(
                    handle,
                    sessionPtr,
                    timestampMs,
                    Int32(limit),
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[Message]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func markRead(
        sessionId: String,
        messageId: String
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatMessageCallback_C()
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

            let result = withCString(sessionId) { sessionPtr in
                withCString(messageId) { messagePtr in
                    anychat_message_mark_read(
                        handle,
                        sessionPtr,
                        messagePtr,
                        &callback
                    )
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Event Streams

    public var receivedMessages: AsyncStream<Message> {
        AsyncStream { continuation in
            Task { await self.setReceivedContinuation(continuation) }
        }
    }

    // MARK: - Listener

    private func setReceivedContinuation(_ continuation: AsyncStream<Message>.Continuation) {
        listenerContext.continuation = continuation
        refreshListener()
    }

    private func refreshListener() {
        guard listenerContext.continuation != nil else {
            anychat_message_set_listener(handle, nil)
            if let userdata = listenerUserdata {
                Unmanaged<MessageListenerContext>.fromOpaque(userdata).release()
                listenerUserdata = nil
            }
            return
        }

        if listenerUserdata == nil {
            listenerUserdata = Unmanaged.passRetained(listenerContext).toOpaque()
        }

        var listener = AnyChatMessageListener_C()
        listener.userdata = listenerUserdata
        listener.on_message_received = { userdata, message in
            guard let userdata, let message else { return }
            let context = Unmanaged<MessageListenerContext>.fromOpaque(userdata).takeUnretainedValue()
            context.continuation?.yield(Message(from: message.pointee))
        }

        anychat_message_set_listener(handle, &listener)
    }
}
