//
//  Group.swift
//  AnyChatSDK
//
//  Group manager with async/await support
//

import Foundation

private final class GroupListenerContext: @unchecked Sendable {
    var invitedContinuation: AsyncStream<(Group, String)>.Continuation?
    var updatedContinuation: AsyncStream<Group>.Continuation?
}

public actor GroupManager {
    private let handle: AnyChatGroupHandle
    private let listenerContext = GroupListenerContext()
    private var listenerUserdata: UnsafeMutableRawPointer?

    init(handle: AnyChatGroupHandle) {
        self.handle = handle
    }

    deinit {
        anychat_group_set_listener(handle, nil)
        if let userdata = listenerUserdata {
            Unmanaged<GroupListenerContext>.fromOpaque(userdata).release()
            listenerUserdata = nil
        }
    }

    // MARK: - Group Operations

    public func getGroupList() async throws -> [Group] {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<[Group], Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatGroupListCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatGroupListCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Group]>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: [])
                    return
                }
                let groups = convertGroupList(list)
                ctx.continuation.resume(returning: groups)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[Group]>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = anychat_group_get_list(handle, &callback)
            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[Group]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func create(name: String, memberIds: [String]) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatGroupInfoCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatGroupInfoCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, _ in
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

            let result = withCString(name) { namePtr in
                withCStringArray(memberIds) { memberPtrs in
                    anychat_group_create(
                        handle,
                        namePtr,
                        memberPtrs,
                        Int32(memberIds.count),
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

    public func join(groupId: String, message: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatGroupCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatGroupCallback_C>.size)
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

            let result = withCString(groupId) { groupPtr in
                withCString(message) { messagePtr in
                    anychat_group_join(handle, groupPtr, messagePtr, &callback)
                }
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func invite(groupId: String, userIds: [String]) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatGroupCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatGroupCallback_C>.size)
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

            let result = withCString(groupId) { groupPtr in
                withCStringArray(userIds) { userPtrs in
                    anychat_group_invite(
                        handle,
                        groupPtr,
                        userPtrs,
                        Int32(userIds.count),
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

    public func quit(groupId: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatGroupCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatGroupCallback_C>.size)
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

            let result = withCString(groupId) { groupPtr in
                anychat_group_quit(handle, groupPtr, &callback)
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<Void>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    public func update(groupId: String, name: String, avatarURL: String) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatGroupCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatGroupCallback_C>.size)
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

            let result = withCString(groupId) { groupPtr in
                withCString(name) { namePtr in
                    withCString(avatarURL) { avatarPtr in
                        anychat_group_update(
                            handle,
                            groupPtr,
                            namePtr,
                            avatarPtr,
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

    public func getMembers(
        groupId: String,
        page: Int = 1,
        pageSize: Int = 20
    ) async throws -> [GroupMember] {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<[GroupMember], Error>) in
            let context = CallbackContext(continuation: continuation)
            let userdata = Unmanaged.passRetained(context).toOpaque()

            var callback = AnyChatGroupMemberListCallback_C()
            callback.struct_size = UInt32(MemoryLayout<AnyChatGroupMemberListCallback_C>.size)
            callback.userdata = userdata
            callback.on_success = { cbUserdata, list in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[GroupMember]>>.fromOpaque(cbUserdata).takeRetainedValue()
                guard let list else {
                    ctx.continuation.resume(returning: [])
                    return
                }
                let members = convertGroupMemberList(list)
                ctx.continuation.resume(returning: members)
                var mutableList = list.pointee
                mutableList.free()
            }
            callback.on_error = { cbUserdata, code, error in
                guard let cbUserdata else { return }
                let ctx = Unmanaged<CallbackContext<[GroupMember]>>.fromOpaque(cbUserdata).takeRetainedValue()
                let message = error != nil ? String(cString: error!) : ""
                ctx.continuation.resume(throwing: AnyChatError(code: Int(code), message: message))
            }

            let result = withCString(groupId) { groupPtr in
                anychat_group_get_members(
                    handle,
                    groupPtr,
                    Int32(page),
                    Int32(pageSize),
                    &callback
                )
            }

            if result != ANYCHAT_OK {
                let ctx = Unmanaged<CallbackContext<[GroupMember]>>.fromOpaque(userdata).takeRetainedValue()
                ctx.continuation.resume(throwing: AnyChatError(code: Int(result), message: getLastError()))
            }
        }
    }

    // MARK: - Event Streams

    public var invited: AsyncStream<(Group, String)> {
        AsyncStream { continuation in
            Task { await self.setInvitedContinuation(continuation) }
        }
    }

    public var updated: AsyncStream<Group> {
        AsyncStream { continuation in
            Task { await self.setUpdatedContinuation(continuation) }
        }
    }

    // MARK: - Listener

    private func setInvitedContinuation(_ continuation: AsyncStream<(Group, String)>.Continuation) {
        listenerContext.invitedContinuation = continuation
        refreshListener()
    }

    private func setUpdatedContinuation(_ continuation: AsyncStream<Group>.Continuation) {
        listenerContext.updatedContinuation = continuation
        refreshListener()
    }

    private func refreshListener() {
        let hasInvited = listenerContext.invitedContinuation != nil
        let hasUpdated = listenerContext.updatedContinuation != nil

        guard hasInvited || hasUpdated else {
            anychat_group_set_listener(handle, nil)
            if let userdata = listenerUserdata {
                Unmanaged<GroupListenerContext>.fromOpaque(userdata).release()
                listenerUserdata = nil
            }
            return
        }

        if listenerUserdata == nil {
            listenerUserdata = Unmanaged.passRetained(listenerContext).toOpaque()
        }

        var listener = AnyChatGroupListener_C()
        listener.struct_size = UInt32(MemoryLayout<AnyChatGroupListener_C>.size)
        listener.userdata = listenerUserdata

        if hasInvited {
            listener.on_group_invited = { userdata, group, inviterId in
                guard let userdata, let group, let inviterId else { return }
                let context = Unmanaged<GroupListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.invitedContinuation?.yield((Group(from: group.pointee), String(cString: inviterId)))
            }
        }

        if hasUpdated {
            listener.on_group_updated = { userdata, group in
                guard let userdata, let group else { return }
                let context = Unmanaged<GroupListenerContext>.fromOpaque(userdata).takeUnretainedValue()
                context.updatedContinuation?.yield(Group(from: group.pointee))
            }
        }

        anychat_group_set_listener(handle, &listener)
    }
}
