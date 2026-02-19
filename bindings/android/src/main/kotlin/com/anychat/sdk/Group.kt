package com.anychat.sdk

import com.anychat.sdk.models.Group as GroupModel
import com.anychat.sdk.models.GroupMember
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine

/**
 * Group manager
 *
 * Provides group-related operations including creation, management, and member operations.
 */
class Group internal constructor(private val handle: Long) {

    /**
     * Get list of groups the current user is a member of
     *
     * @return List of groups
     */
    suspend fun getList(): List<GroupModel> = suspendCoroutine { continuation ->
        nativeGetList(handle, object : GroupListCallback {
            override fun onGroupList(groups: List<GroupModel>?, error: String?) {
                if (groups != null) {
                    continuation.resume(groups)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Get group list failed")
                    )
                }
            }
        })
    }

    /**
     * Create a new group
     *
     * @param name Group name
     * @param memberIds Array of user IDs to add as initial members
     */
    suspend fun create(
        name: String,
        memberIds: Array<String>
    ): Unit = suspendCoroutine { continuation ->
        nativeCreate(handle, name, memberIds, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Create group failed")
                    )
                }
            }
        })
    }

    /**
     * Join a group
     *
     * @param groupId Group ID to join
     * @param message Optional message to the group admin
     */
    suspend fun join(
        groupId: String,
        message: String = ""
    ): Unit = suspendCoroutine { continuation ->
        nativeJoin(handle, groupId, message, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Join group failed")
                    )
                }
            }
        })
    }

    /**
     * Invite users to a group
     *
     * @param groupId Group ID
     * @param userIds Array of user IDs to invite
     */
    suspend fun invite(
        groupId: String,
        userIds: Array<String>
    ): Unit = suspendCoroutine { continuation ->
        nativeInvite(handle, groupId, userIds, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Invite to group failed")
                    )
                }
            }
        })
    }

    /**
     * Quit/leave a group
     *
     * @param groupId Group ID to quit
     */
    suspend fun quit(groupId: String): Unit = suspendCoroutine { continuation ->
        nativeQuit(handle, groupId, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Quit group failed")
                    )
                }
            }
        })
    }

    /**
     * Update group information
     *
     * @param groupId Group ID
     * @param name New group name (optional)
     * @param avatarUrl New avatar URL (optional)
     */
    suspend fun update(
        groupId: String,
        name: String? = null,
        avatarUrl: String? = null
    ): Unit = suspendCoroutine { continuation ->
        nativeUpdate(
            handle,
            groupId,
            name ?: "",
            avatarUrl ?: "",
            object : ResultCallback {
                override fun onResult(success: Boolean, error: String?) {
                    if (success) {
                        continuation.resume(Unit)
                    } else {
                        continuation.resumeWithException(
                            RuntimeException(error ?: "Update group failed")
                        )
                    }
                }
            })
    }

    /**
     * Get group members with pagination
     *
     * @param groupId Group ID
     * @param page Page number (starts from 1)
     * @param pageSize Number of members per page
     * @return List of group members
     */
    suspend fun getMembers(
        groupId: String,
        page: Int = 1,
        pageSize: Int = 20
    ): List<GroupMember> = suspendCoroutine { continuation ->
        nativeGetMembers(handle, groupId, page, pageSize, object : GroupMemberListCallback {
            override fun onGroupMemberList(members: List<GroupMember>?, error: String?) {
                if (members != null) {
                    continuation.resume(members)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Get group members failed")
                    )
                }
            }
        })
    }

    // Native methods
    private external fun nativeGetList(handle: Long, callback: GroupListCallback)

    private external fun nativeCreate(
        handle: Long,
        name: String,
        memberIds: Array<String>,
        callback: ResultCallback
    )

    private external fun nativeJoin(
        handle: Long,
        groupId: String,
        message: String,
        callback: ResultCallback
    )

    private external fun nativeInvite(
        handle: Long,
        groupId: String,
        userIds: Array<String>,
        callback: ResultCallback
    )

    private external fun nativeQuit(
        handle: Long,
        groupId: String,
        callback: ResultCallback
    )

    private external fun nativeUpdate(
        handle: Long,
        groupId: String,
        name: String,
        avatarUrl: String,
        callback: ResultCallback
    )

    private external fun nativeGetMembers(
        handle: Long,
        groupId: String,
        page: Int,
        pageSize: Int,
        callback: GroupMemberListCallback
    )
}
