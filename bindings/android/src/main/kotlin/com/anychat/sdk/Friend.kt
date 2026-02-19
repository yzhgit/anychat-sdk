package com.anychat.sdk

import com.anychat.sdk.models.Friend as FriendModel
import com.anychat.sdk.models.FriendRequest
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine

/**
 * Friend manager
 *
 * Provides friend-related operations including friend list, requests, and management.
 */
class Friend internal constructor(private val handle: Long) {

    /**
     * Get friend list
     *
     * @return List of friends
     */
    suspend fun getList(): List<FriendModel> = suspendCoroutine { continuation ->
        nativeGetList(handle, object : FriendListCallback {
            override fun onFriendList(friends: List<FriendModel>?, error: String?) {
                if (friends != null) {
                    continuation.resume(friends)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Get friend list failed")
                    )
                }
            }
        })
    }

    /**
     * Send a friend request
     *
     * @param toUserId Target user ID
     * @param message Optional message to include with the request
     */
    suspend fun sendRequest(
        toUserId: String,
        message: String = ""
    ): Unit = suspendCoroutine { continuation ->
        nativeSendRequest(handle, toUserId, message, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Send friend request failed")
                    )
                }
            }
        })
    }

    /**
     * Handle a friend request (accept or reject)
     *
     * @param requestId Friend request ID
     * @param accept True to accept, false to reject
     */
    suspend fun handleRequest(
        requestId: Long,
        accept: Boolean
    ): Unit = suspendCoroutine { continuation ->
        nativeHandleRequest(handle, requestId, accept, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Handle friend request failed")
                    )
                }
            }
        })
    }

    /**
     * Get pending friend requests
     *
     * @return List of pending friend requests
     */
    suspend fun getPendingRequests(): List<FriendRequest> = suspendCoroutine { continuation ->
        nativeGetPendingRequests(handle, object : FriendRequestListCallback {
            override fun onFriendRequestList(requests: List<FriendRequest>?, error: String?) {
                if (requests != null) {
                    continuation.resume(requests)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Get pending requests failed")
                    )
                }
            }
        })
    }

    /**
     * Delete a friend
     *
     * @param friendId Friend user ID
     */
    suspend fun delete(friendId: String): Unit = suspendCoroutine { continuation ->
        nativeDelete(handle, friendId, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Delete friend failed")
                    )
                }
            }
        })
    }

    /**
     * Update friend remark/alias
     *
     * @param friendId Friend user ID
     * @param remark New remark name
     */
    suspend fun updateRemark(
        friendId: String,
        remark: String
    ): Unit = suspendCoroutine { continuation ->
        nativeUpdateRemark(handle, friendId, remark, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Update remark failed")
                    )
                }
            }
        })
    }

    // Native methods
    private external fun nativeGetList(handle: Long, callback: FriendListCallback)

    private external fun nativeSendRequest(
        handle: Long,
        toUserId: String,
        message: String,
        callback: ResultCallback
    )

    private external fun nativeHandleRequest(
        handle: Long,
        requestId: Long,
        accept: Boolean,
        callback: ResultCallback
    )

    private external fun nativeGetPendingRequests(
        handle: Long,
        callback: FriendRequestListCallback
    )

    private external fun nativeDelete(
        handle: Long,
        friendId: String,
        callback: ResultCallback
    )

    private external fun nativeUpdateRemark(
        handle: Long,
        friendId: String,
        remark: String,
        callback: ResultCallback
    )
}
