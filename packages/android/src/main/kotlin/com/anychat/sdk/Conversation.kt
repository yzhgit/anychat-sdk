package com.anychat.sdk

import com.anychat.sdk.models.Conversation as ConversationModel
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine

/**
 * Conversation manager
 *
 * Provides conversation-related operations including list management and updates.
 */
class Conversation internal constructor(private val handle: Long) {

    /**
     * Get list of all conversations
     *
     * Returns cached + DB list, sorted by pinned first, then by last message time descending.
     *
     * @return List of conversations
     */
    suspend fun getList(): List<ConversationModel> = suspendCoroutine { continuation ->
        nativeGetList(handle, object : ConversationListCallback {
            override fun onConversationList(conversations: List<ConversationModel>?, error: String?) {
                if (conversations != null) {
                    continuation.resume(conversations)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Get conversation list failed")
                    )
                }
            }
        })
    }

    /**
     * Mark a conversation as read
     *
     * Clears the unread count for the conversation.
     *
     * @param convId Conversation ID
     */
    suspend fun markRead(convId: String): Unit = suspendCoroutine { continuation ->
        nativeMarkRead(handle, convId, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Mark read failed")
                    )
                }
            }
        })
    }

    /**
     * Pin or unpin a conversation
     *
     * Pinned conversations appear at the top of the conversation list.
     *
     * @param convId Conversation ID
     * @param pinned True to pin, false to unpin
     */
    suspend fun setPinned(convId: String, pinned: Boolean): Unit = suspendCoroutine { continuation ->
        nativeSetPinned(handle, convId, pinned, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Set pinned failed")
                    )
                }
            }
        })
    }

    /**
     * Mute or unmute a conversation
     *
     * Muted conversations don't trigger notifications.
     *
     * @param convId Conversation ID
     * @param muted True to mute, false to unmute
     */
    suspend fun setMuted(convId: String, muted: Boolean): Unit = suspendCoroutine { continuation ->
        nativeSetMuted(handle, convId, muted, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Set muted failed")
                    )
                }
            }
        })
    }

    /**
     * Delete a conversation
     *
     * Deletes both locally and on the server.
     *
     * @param convId Conversation ID
     */
    suspend fun delete(convId: String): Unit = suspendCoroutine { continuation ->
        nativeDelete(handle, convId, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Delete conversation failed")
                    )
                }
            }
        })
    }

    /**
     * Flow of conversation updates
     *
     * Emits when a conversation is created or updated.
     */
    val conversationUpdateFlow: Flow<ConversationModel> = callbackFlow {
        val callback = ConversationUpdatedCallback { conversation ->
            trySend(conversation)
        }
        nativeSetUpdatedCallback(handle, callback)

        awaitClose {
            nativeSetUpdatedCallback(handle, null)
        }
    }

    /**
     * Set a callback for conversation updates (alternative to Flow)
     *
     * @param callback Callback to invoke when a conversation is updated, or null to clear
     */
    fun setUpdatedCallback(callback: ConversationUpdatedCallback?) {
        nativeSetUpdatedCallback(handle, callback)
    }

    // Native methods
    private external fun nativeGetList(handle: Long, callback: ConversationListCallback)

    private external fun nativeMarkRead(
        handle: Long,
        convId: String,
        callback: ResultCallback
    )

    private external fun nativeSetPinned(
        handle: Long,
        convId: String,
        pinned: Boolean,
        callback: ResultCallback
    )

    private external fun nativeSetMuted(
        handle: Long,
        convId: String,
        muted: Boolean,
        callback: ResultCallback
    )

    private external fun nativeDelete(
        handle: Long,
        convId: String,
        callback: ResultCallback
    )

    private external fun nativeSetUpdatedCallback(
        handle: Long,
        callback: ConversationUpdatedCallback?
    )
}
