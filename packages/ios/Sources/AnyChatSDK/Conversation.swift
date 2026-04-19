//
//  Conversation.swift
//  AnyChatSDK
//
//  Conversation manager with async/await support
//

import Foundation

private final class ConversationListenerContext: @unchecked Sendable {
    var continuation: AsyncStream<Conversation>.Continuation?
}

public actor ConversationManager {
    private let handle: AnyChatConvHandle
    private let listenerContext = ConversationListenerContext()
    private var listenerUserdata: UnsafeMutableRawPointer?

    init(handle: AnyChatConvHandle) {
        self.handle = handle
    }

    deinit {
        anychat_conv_set_listener(handle, nil)
        if let userdata = listenerUserdata {
            Unmanaged<ConversationListenerContext>.fromOpaque(userdata).release()
            listenerUserdata = nil
        }
    }

    // MARK: - Conversation Operations

    public func getConversationList() async throws -> [Conversation] {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<[Conversation], Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatConvListCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Conversation]>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: [])
                    return
                }
                let conversations = convertConversationList(list)
                ctx.continuation.resume(returning: conversations)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Conversation]>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_conv_get_list(handle, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[Conversation]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func markRead(conversationId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatConvCallback_C()
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

            let result = withCString(conversationId) { convPtr in
                anychat_conv_mark_all_read(handle, convPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func setPinned(
        conversationId: String,
        pinned: Bool
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatConvCallback_C()
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

            let result = withCString(conversationId) { convPtr in
                anychat_conv_set_pinned(
                    handle,
                    convPtr,
                    pinned ? 1 : 0,
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func setMuted(
        conversationId: String,
        muted: Bool
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatConvCallback_C()
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

            let result = withCString(conversationId) { convPtr in
                anychat_conv_set_muted(
                    handle,
                    convPtr,
                    muted ? 1 : 0,
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func delete(conversationId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatConvCallback_C()
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

            let result = withCString(conversationId) { convPtr in
                anychat_conv_delete(handle, convPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Event Streams

    public var conversationUpdated: AsyncStream<Conversation> {
        AsyncStream { continuation in
            Task { await self.setUpdatedContinuation(continuation) }
        }
    }

    // MARK: - Listener

    private func setUpdatedContinuation(_ continuation: AsyncStream<Conversation>.Continuation) {
        listenerContext.continuation = continuation
        refreshListener()
    }

    private func refreshListener() {
        guard listenerContext.continuation != nil else {
            anychat_conv_set_listener(handle, nil)
            if let userdata = listenerUserdata {
                Unmanaged<ConversationListenerContext>.fromOpaque(userdata).release()
                listenerUserdata = nil
            }
            return
        }

        if listenerUserdata == nil {
            listenerUserdata = Unmanaged.passRetained(listenerContext).toOpaque()
        }

        var listener = AnyChatConvListener_C()
        listener.userdata = listenerUserdata
        listener.on_conversation_updated = { userdata, conversation in
            guard let userdata, let conversation else { return }
            let context = Unmanaged<ConversationListenerContext>.fromOpaque(userdata).takeUnretainedValue()
            context.continuation?.yield(Conversation(from: conversation.pointee))
        }

        anychat_conv_set_listener(handle, &listener)
    }
}
