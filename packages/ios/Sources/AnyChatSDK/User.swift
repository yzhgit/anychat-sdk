//
//  User.swift
//  AnyChatSDK
//
//  User manager with async/await support
//

import Foundation

public actor UserManager {
    private let handle: AnyChatUserHandle

    init(handle: AnyChatUserHandle) {
        self.handle = handle
    }

    // MARK: - Profile Operations

    public func getProfile() async throws -> UserProfile {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<UserProfile, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatUserProfileCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, profile in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let profile else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: UserProfile(from: profile.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_user_get_profile(handle, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func updateProfile(_ profile: UserProfile) async throws -> UserProfile {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<UserProfile, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatUserProfileCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, profile in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let profile else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: UserProfile(from: profile.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            var cProfile = profile.toCStruct()
            let result = anychat_user_update_profile(handle, &cProfile, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Settings Operations

    public func getSettings() async throws -> UserSettings {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<UserSettings, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatUserSettingsCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, settings in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let settings else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: UserSettings(from: settings.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_user_get_settings(handle, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func updateSettings(_ settings: UserSettings) async throws -> UserSettings {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<UserSettings, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatUserSettingsCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, settings in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let settings else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: UserSettings(from: settings.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            var cSettings = settings.toCStruct()
            let result = anychat_user_update_settings(handle, &cSettings, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Push Token

    public func updatePushToken(
        token: String,
        platform: Int32 = Int32(ANYCHAT_PUSH_PLATFORM_IOS)
    ) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatUserCallback_C()
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

            let result = withCString(token) { tokenPtr in
                anychat_user_update_push_token(
                    handle,
                    tokenPtr,
                    platform,
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - User Search

    public func search(
        keyword: String,
        page: Int = 1,
        pageSize: Int = 20
    ) async throws -> ([UserInfo], Int64) {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<([UserInfo], Int64), Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatUserListCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<([UserInfo], Int64)>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: ([], 0))
                    return
                }
                let result = convertUserList(list)
                ctx.continuation.resume(returning: result)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<([UserInfo], Int64)>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(keyword) { keywordPtr in
                anychat_user_search(
                    handle,
                    keywordPtr,
                    Int32(page),
                    Int32(pageSize),
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<([UserInfo], Int64)>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func getInfo(userId: String) async throws -> UserInfo {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<UserInfo, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatUserInfoCallback_C()
            callback.userdata = userdata
            callback.on_success = { cbUserdata, info in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserInfo>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let info else {
                    ctx.continuation.resume(throwing: AnyChatError.network)
                    return
                }
                ctx.continuation.resume(returning: UserInfo(from: info.pointee))
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<UserInfo>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(userId) { userIdPtr in
                anychat_user_get_info(handle, userIdPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserInfo>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }
}
