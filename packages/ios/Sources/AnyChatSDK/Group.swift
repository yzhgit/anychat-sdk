//
//  Group.swift
//  AnyChatSDK
//
//  Group manager with async/await support
//

import Foundation

public actor GroupManager {
    private let handle: AnyChatGroupHandle
    private var invitedContinuation: AsyncStream<(Group, String)>.Continuation?
    private var updatedContinuation: AsyncStream<Group>.Continuation?

    init(handle: AnyChatGroupHandle) {
        self.handle = handle
        setupCallbacks()
    }

    // MARK: - Group Operations

    public func getList() async throws -> [Group] {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatGroupListCallback = { userdata, list, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<[Group]>>.fromOpaque(userdata).takeRetainedValue()

                if let list = list {
                    let groups = convertGroupList(list)
                    context.continuation.resume(returning: groups)
                    var mutableList = list.pointee
                    mutableList.free()
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to fetch groups"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            let result = anychat_group_get_list(handle, userdata, callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[Group]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
            }
        }
    }

    public func create(name: String, memberIds: [String]) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatGroupCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Create group failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(name) { namePtr in
                withCStringArray(memberIds) { memberPtrs in
                    let result = anychat_group_create(
                        handle,
                        namePtr,
                        memberPtrs,
                        Int32(memberIds.count),
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

    public func join(groupId: String, message: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatGroupCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Join group failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(groupId) { groupPtr in
                withCString(message) { msgPtr in
                    let result = anychat_group_join(handle, groupPtr, msgPtr, userdata, callback)

                    if result != ANYCHAT_OK {
                        let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                        ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                    }
                }
            }
        }
    }

    public func invite(groupId: String, userIds: [String]) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatGroupCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Invite failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(groupId) { groupPtr in
                withCStringArray(userIds) { userPtrs in
                    let result = anychat_group_invite(
                        handle,
                        groupPtr,
                        userPtrs,
                        Int32(userIds.count),
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

    public func quit(groupId: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatGroupCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Quit group failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(groupId) { groupPtr in
                let result = anychat_group_quit(handle, groupPtr, userdata, callback)

                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    public func update(groupId: String, name: String, avatarURL: String) async throws {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatGroupCallback = { userdata, success, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()

                if success != 0 {
                    context.continuation.resume(returning: ())
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Update group failed"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(groupId) { groupPtr in
                withCString(name) { namePtr in
                    withCString(avatarURL) { avatarPtr in
                        let result = anychat_group_update(
                            handle,
                            groupPtr,
                            namePtr,
                            avatarPtr,
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
    }

    public func getMembers(
        groupId: String,
        page: Int = 1,
        pageSize: Int = 20
    ) async throws -> [GroupMember] {
        try await withCheckedThrowingContinuation { continuation in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            let callback: AnyChatGroupMemberCallback = { userdata, list, error in
                guard let userdata = userdata else { return }
                let context = Unmanaged<CallbackContext<[GroupMember]>>.fromOpaque(userdata).takeRetainedValue()

                if let list = list {
                    let members = convertGroupMemberList(list)
                    context.continuation.resume(returning: members)
                    var mutableList = list.pointee
                    mutableList.free()
                } else {
                    let errorMsg = error != nil ? String(cString: error!) : "Failed to get members"
                    context.continuation.resume(throwing: AnyChatError.network)
                }
            }

            withCString(groupId) { groupPtr in
                let result = anychat_group_get_members(
                    handle,
                    groupPtr,
                    Int32(page),
                    Int32(pageSize),
                    userdata,
                    callback
                )

                if result != ANYCHAT_OK {
                    let ctx = Unmanaged<CallbackContext<[GroupMember]>>.fromOpaque(userdata).takeRetainedValue()
                    ctx.continuation.resume(throwing: AnyChatError(code: Int(result)))
                }
            }
        }
    }

    // MARK: - Event Streams

    public var invited: AsyncStream<(Group, String)> {
        AsyncStream { continuation in
            self.invitedContinuation = continuation
        }
    }

    public var updated: AsyncStream<Group> {
        AsyncStream { continuation in
            self.updatedContinuation = continuation
        }
    }

    // MARK: - Private

    private func setupCallbacks() {
        let invitedCallback: AnyChatGroupInvitedCallback = { userdata, group, inviterId in
            guard let userdata = userdata, let group = group, let inviterId = inviterId else { return }
            let context = Unmanaged<StreamContext<(Group, String)>>.fromOpaque(userdata).takeUnretainedValue()
            let grp = Group(from: group.pointee)
            let inviter = String(cString: inviterId)
            context.continuation.yield((grp, inviter))
        }

        let updatedCallback: AnyChatGroupUpdatedCallback = { userdata, group in
            guard let userdata = userdata, let group = group else { return }
            let context = Unmanaged<StreamContext<Group>>.fromOpaque(userdata).takeUnretainedValue()
            context.continuation.yield(Group(from: group.pointee))
        }

        Task {
            if let cont = invitedContinuation {
                let context = StreamContext(continuation: cont)
                let userdata = Unmanaged.passRetained(context).toOpaque()
                anychat_group_set_invited_callback(handle, userdata, invitedCallback)
            }

            if let cont = updatedContinuation {
                let context = StreamContext(continuation: cont)
                let userdata = Unmanaged.passRetained(context).toOpaque()
                anychat_group_set_updated_callback(handle, userdata, updatedCallback)
            }
        }
    }
}
