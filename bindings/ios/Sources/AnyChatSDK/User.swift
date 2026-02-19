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
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatUserProfileCallback = { userdata, success, profile, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0, let profile = profile?.pointee {
                    context.continuation.resume(returning: UserProfile(from: profile))
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to get profile"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            let result = anychat_user_get_profile(handle, userdata, callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    public func updateProfile(_ profile: UserProfile) async throws -> UserProfile {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatUserProfileCallback = { userdata, success, profile, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0, let profile = profile?.pointee {
                    context.continuation.resume(returning: UserProfile(from: profile))
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to update profile"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            var cProfile = profile.toCStruct()
            let result = anychat_user_update_profile(handle, &cProfile, userdata, callback)

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserProfile>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    // MARK: - Settings Operations

    public func getSettings() async throws -> UserSettings {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatUserSettingsCallback = { userdata, success, settings, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0, let settings = settings?.pointee {
                    context.continuation.resume(returning: UserSettings(from: settings))
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to get settings"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            let result = anychat_user_get_settings(handle, userdata, callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    public func updateSettings(_ settings: UserSettings) async throws -> UserSettings {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatUserSettingsCallback = { userdata, success, settings, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0, let settings = settings?.pointee {
                    context.continuation.resume(returning: UserSettings(from: settings))
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to update settings"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            var cSettings = settings.toCStruct()
            let result = anychat_user_update_settings(handle, &cSettings, userdata, callback)

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<UserSettings>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    // MARK: - Push Token

    public func updatePushToken(
        token: String,
        platform: String = "ios"
    ) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatUserResultCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to update push token"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(token) { tokenPtr in
                withCString(platform) { platformPtr in
                    let result = anychat_user_update_push_token(
                        handle,
                        tokenPtr,
                        platformPtr,
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

    // MARK: - User Search

    public func search(
        keyword: String,
        page: Int = 1,
        pageSize: Int = 20
    ) async throws -> ([UserInfo], Int64) {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatUserListCallback = { userdata, list, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<([UserInfo], Int64)>>.fromOpaque(userdata).takeRetainedValue()

                if let list = list {
                    let result = convertUserList(list)
                    context.continuation.resume(returning: result)
                    var mutableList = list.pointee
                    mutableList.free()
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Search failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(keyword) { keywordPtr in
                let result = anychat_user_search(
                    handle,
                    keywordPtr,
                    Int32(page),
                    Int32(pageSize),
                    userdata,
                    callback
                )

                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<([UserInfo], Int64)>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    public func getInfo(userId: String) async throws -> UserInfo {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatUserInfoCallback = { userdata, success, info, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<UserInfo>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0, let info = info?.pointee {
                    context.continuation.resume(returning: UserInfo(from: info))
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to get user info"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(userId) { userIdPtr in
                let result = anychat_user_get_info(handle, userIdPtr, userdata, callback)

                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<UserInfo>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }
}
