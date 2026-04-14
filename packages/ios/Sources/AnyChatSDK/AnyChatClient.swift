//
//  AnyChatClient.swift
//  AnyChatSDK
//
//  Main entry point for the AnyChatSDK
//

import Foundation

private final class ConnectionStateStreamContext: @unchecked Sendable {
    var continuation: AsyncStream<ConnectionState>.Continuation?
}

public actor AnyChatClient {
    private let handleWrapper: ClientHandleWrapper
    private let connectionContext = ConnectionStateStreamContext()
    private var connectionContextPointer: UnsafeMutableRawPointer?

    public let auth: AuthManager
    public let message: MessageManager
    public let conversation: ConversationManager
    public let friend: FriendManager
    public let group: GroupManager
    public let user: UserManager
    public let file: FileManager
    public let call: CallManager

    // MARK: - Initialization

    public init(config: ClientConfig) throws {
        var cConfig = AnyChatClientConfig_C()
        cConfig.gateway_url = nil
        cConfig.api_base_url = nil
        cConfig.device_id = nil
        cConfig.db_path = nil
        cConfig.connect_timeout_ms = Int32(config.connectTimeoutMs)
        cConfig.max_reconnect_attempts = Int32(config.maxReconnectAttempts)
        cConfig.auto_reconnect = config.autoReconnect ? 1 : 0

        let handle = withCString(config.gatewayURL) { gatewayPtr in
            withCString(config.apiBaseURL) { apiPtr in
                withCString(config.deviceId) { devicePtr in
                    withCString(config.dbPath) { dbPtr in
                        var cfg = cConfig
                        cfg.gateway_url = gatewayPtr
                        cfg.api_base_url = apiPtr
                        cfg.device_id = devicePtr
                        cfg.db_path = dbPtr
                        return anychat_client_create(&cfg)
                    }
                }
            }
        }

        guard let handle else {
            throw AnyChatError.unknown(code: -1, message: getLastError())
        }

        self.handleWrapper = ClientHandleWrapper(handle: handle)

        guard
            let authHandle = anychat_client_get_auth(handle),
            let messageHandle = anychat_client_get_message(handle),
            let convHandle = anychat_client_get_conversation(handle),
            let friendHandle = anychat_client_get_friend(handle),
            let groupHandle = anychat_client_get_group(handle),
            let userHandle = anychat_client_get_user(handle),
            let fileHandle = anychat_client_get_file(handle),
            let callHandle = anychat_client_get_call(handle)
        else {
            throw AnyChatError.unknown(code: -1, message: "Failed to get sub-module handles")
        }

        self.auth = AuthManager(handle: authHandle, clientHandle: handle)
        self.message = MessageManager(handle: messageHandle)
        self.conversation = ConversationManager(handle: convHandle)
        self.friend = FriendManager(handle: friendHandle)
        self.group = GroupManager(handle: groupHandle)
        self.user = UserManager(handle: userHandle)
        self.file = FileManager(handle: fileHandle)
        self.call = CallManager(handle: callHandle)

        setupConnectionCallback()
    }

    deinit {
        anychat_client_set_connection_callback(handleWrapper.handle, nil, nil)
        if let pointer = connectionContextPointer {
            Unmanaged<ConnectionStateStreamContext>.fromOpaque(pointer).release()
            connectionContextPointer = nil
        }
    }

    // MARK: - Connection

    public func getConnectionState() -> ConnectionState {
        let state = anychat_client_get_connection_state(handleWrapper.handle)
        return ConnectionState(rawValue: Int(state)) ?? .disconnected
    }

    public var connectionState: AsyncStream<ConnectionState> {
        AsyncStream { continuation in
            Task { await self.setConnectionStateContinuation(continuation) }
        }
    }

    // MARK: - Private

    private func setupConnectionCallback() {
        let pointer = Unmanaged.passRetained(connectionContext).toOpaque()
        connectionContextPointer = pointer

        let callback: AnyChatConnectionStateCallback = { userdata, state in
            guard let userdata else { return }
            let context = Unmanaged<ConnectionStateStreamContext>.fromOpaque(userdata).takeUnretainedValue()
            guard let mapped = ConnectionState(rawValue: Int(state)) else { return }
            context.continuation?.yield(mapped)
        }

        anychat_client_set_connection_callback(handleWrapper.handle, pointer, callback)
    }

    private func setConnectionStateContinuation(_ continuation: AsyncStream<ConnectionState>.Continuation) {
        connectionContext.continuation = continuation
    }
}
