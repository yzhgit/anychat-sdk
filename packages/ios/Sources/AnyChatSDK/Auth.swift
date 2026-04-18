//
//  Auth.swift
//  AnyChatSDK
//
//  Authentication manager with async/await support
//

import Foundation

private final class AuthListenerContext: @unchecked Sendable {
    var continuation: AsyncStream<Void>.Continuation?
}

public actor AuthManager {
    private let handle: AnyChatAuthHandle
    private let clientHandle: AnyChatClientHandle?
    private let listenerContext = AuthListenerContext()
    private var listenerUserdata: UnsafeMutableRawPointer?

    init(handle: AnyChatAuthHandle, clientHandle: AnyChatClientHandle? = nil) {
        self.handle = handle
        self.clientHandle = clientHandle
    }

    deinit {
        anychat_auth_set_listener(handle, nil)
        if let userdata = listenerUserdata {
            Unmanaged<AuthListenerContext>.fromOpaque(userdata).release()
            listenerUserdata = nil
        }
    }

    // MARK: - Authentication Operations

    public func login(
        account: String,
        password: String,
        deviceType: Int32 = 1,
        clientVersion: String = ""
    ) async throws -> AuthToken {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<AuthToken, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthTokenCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthTokenCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, token in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let token else {
                    ctx.continuation.resume(throwing: AnyChatError.auth)
                    return
                }
                ctx.continuation.resume(returning: AuthToken(from: token.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(account) { accountPtr in
                withCString(password) { passwordPtr in
                    withOptionalCString(clientVersion.isEmpty ? nil : clientVersion) { clientVersionPtr in
                        if let clientHandle {
                            return anychat_client_login(
                                clientHandle,
                                accountPtr,
                                passwordPtr,
                                deviceType,
                                clientVersionPtr,
                                &callback
                            )
                        }
                        return anychat_auth_login(
                            handle,
                            accountPtr,
                            passwordPtr,
                            deviceType,
                            clientVersionPtr,
                            &callback
                        )
                    }
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func register(
        phoneOrEmail: String,
        password: String,
        verifyCode: String,
        deviceType: Int32 = 1,
        nickname: String? = nil,
        clientVersion: String = ""
    ) async throws -> AuthToken {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<AuthToken, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthTokenCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthTokenCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, token in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let token else {
                    ctx.continuation.resume(throwing: AnyChatError.auth)
                    return
                }
                ctx.continuation.resume(returning: AuthToken(from: token.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(phoneOrEmail) { phonePtr in
                withCString(password) { passwordPtr in
                    withCString(verifyCode) { codePtr in
                        withOptionalCString(nickname) { nicknamePtr in
                            withOptionalCString(clientVersion.isEmpty ? nil : clientVersion) { clientVersionPtr in
                                anychat_auth_register(
                                    handle,
                                    phonePtr,
                                    passwordPtr,
                                    codePtr,
                                    deviceType,
                                    nicknamePtr,
                                    clientVersionPtr,
                                    &callback
                                )
                            }
                        }
                    }
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func sendCode(
        target: String,
        targetType: Int32,
        purpose: Int32
    ) async throws -> VerificationCodeResult {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<VerificationCodeResult, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatVerificationCodeCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatVerificationCodeCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, result in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<VerificationCodeResult>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let result else {
                    ctx.continuation.resume(throwing: AnyChatError.auth)
                    return
                }
                ctx.continuation.resume(returning: VerificationCodeResult(from: result.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<VerificationCodeResult>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(target) { targetPtr in
                anychat_auth_send_code(
                    handle,
                    targetPtr,
                    targetType,
                    purpose,
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<VerificationCodeResult>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func logout() async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthResultCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthResultCallback_C>.size)
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

            let result: Int32
            if let clientHandle {
                result = anychat_client_logout(clientHandle, &callback)
            } else {
                result = anychat_auth_logout(handle, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func refreshToken(_ refreshToken: String) async throws -> AuthToken {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<AuthToken, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthTokenCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthTokenCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, token in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let token else {
                    ctx.continuation.resume(throwing: AnyChatError.tokenExpired)
                    return
                }
                ctx.continuation.resume(returning: AuthToken(from: token.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(refreshToken) { tokenPtr in
                anychat_auth_refresh_token(handle, tokenPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<AuthToken>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func changePassword(
        oldPassword: String,
        newPassword: String
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthResultCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthResultCallback_C>.size)
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

            let result = withCString(oldPassword) { oldPtr in
                withCString(newPassword) { newPtr in
                    anychat_auth_change_password(handle, oldPtr, newPtr, &callback)
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func resetPassword(
        account: String,
        verifyCode: String,
        newPassword: String
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthResultCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthResultCallback_C>.size)
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

            let result = withCString(account) { accountPtr in
                withCString(verifyCode) { verifyCodePtr in
                    withCString(newPassword) { newPasswordPtr in
                        anychat_auth_reset_password(
                            handle,
                            accountPtr,
                            verifyCodePtr,
                            newPasswordPtr,
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

    public func getDeviceList() async throws -> [AuthDevice] {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<[AuthDevice], Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthDeviceListCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthDeviceListCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[AuthDevice]>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: [])
                    return
                }
                let devices = convertAuthDeviceList(list)
                ctx.continuation.resume(returning: devices)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[AuthDevice]>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_auth_get_device_list(handle, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[AuthDevice]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func logoutDevice(deviceId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatAuthResultCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatAuthResultCallback_C>.size)
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

            let result = withCString(deviceId) { deviceIdPtr in
                anychat_auth_logout_device(handle, deviceIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - State Queries

    public func isLoggedIn() -> Bool {
        if let clientHandle {
            return anychat_client_is_logged_in(clientHandle) != 0
        }
        return anychat_auth_is_logged_in(handle) != 0
    }

    public func getCurrentToken() throws -> AuthToken {
        var cToken = AnyChatAuthToken_C()
        let result: Int32
        if let clientHandle {
            result = anychat_client_get_current_token(clientHandle, &cToken)
        } else {
            result = anychat_auth_get_current_token(handle, &cToken)
        }
        try checkResult(result)
        return AuthToken(from: cToken)
    }

    // MARK: - Event Streams

    public var tokenExpired: AsyncStream<Void> {
        AsyncStream { continuation in
            Task { await self.setTokenExpiredContinuation(continuation) }
        }
    }

    // MARK: - Listener

    private func setTokenExpiredContinuation(_ continuation: AsyncStream<Void>.Continuation) {
        listenerContext.continuation = continuation
        refreshListener()
    }

    private func refreshListener() {
        guard listenerContext.continuation != nil else {
            anychat_auth_set_listener(handle, nil)
            if let userdata = listenerUserdata {
                Unmanaged<AuthListenerContext>.fromOpaque(userdata).release()
                listenerUserdata = nil
            }
            return
        }

        if listenerUserdata == nil {
            listenerUserdata = Unmanaged.passRetained(listenerContext).toOpaque()
        }

        var listener = AnyChatAuthListener_C()
        listener.struct_size = UInt32(MemoryLayout<AnyChatAuthListener_C>.size)
        listener.userdata = listenerUserdata
        listener.on_auth_expired = { userdata in
            guard let userdata else { return }
            let context = Unmanaged<AuthListenerContext>.fromOpaque(userdata).takeUnretainedValue()
            context.continuation?.yield(())
        }

        anychat_auth_set_listener(handle, &listener)
    }
}
