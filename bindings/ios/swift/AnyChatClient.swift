import Foundation

/// Swift wrapper around the SWIG-generated Objective-C bindings.
/// Provides an async/await API for iOS developers.
public class AnyChatClient {

    private let config: ClientConfig

    public init(config: ClientConfig) {
        self.config = config
    }

    // TODO: delegate to SWIG-generated ObjC wrapper
    public func connect() async throws {
        throw AnyChatError.notImplemented
    }

    public func disconnect() {
        // TODO
    }
}

public struct ClientConfig {
    public let gatewayUrl: String
    public let apiBaseUrl: String
    public var connectTimeoutMs: Int = 10_000
    public var autoReconnect: Bool = true

    public init(gatewayUrl: String, apiBaseUrl: String) {
        self.gatewayUrl = gatewayUrl
        self.apiBaseUrl = apiBaseUrl
    }
}

public enum AnyChatError: Error {
    case notImplemented
    case connectionFailed(String)
    case authFailed(String)
}
