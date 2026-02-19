package com.anychat.sdk

import com.anychat.sdk.models.Message as MessageModel
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine

/**
 * Message manager
 *
 * Provides message-related operations including sending, receiving, and history management.
 */
class Message internal constructor(private val handle: Long) {

    /**
     * Send a text message to a conversation
     *
     * @param sessionId Conversation ID
     * @param content Message content
     */
    suspend fun sendText(
        sessionId: String,
        content: String
    ): Unit = suspendCoroutine { continuation ->
        nativeSendText(handle, sessionId, content, object : ResultCallback {
            override fun onResult(success: Boolean, error: String?) {
                if (success) {
                    continuation.resume(Unit)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Send message failed")
                    )
                }
            }
        })
    }

    /**
     * Get message history for a conversation
     *
     * @param sessionId Conversation ID
     * @param beforeTimestampMs Get messages before this timestamp (0 = most recent)
     * @param limit Maximum number of messages to return
     * @return List of messages
     */
    suspend fun getHistory(
        sessionId: String,
        beforeTimestampMs: Long = 0,
        limit: Int = 20
    ): List<MessageModel> = suspendCoroutine { continuation ->
        nativeGetHistory(handle, sessionId, beforeTimestampMs, limit, object : MessageListCallback {
            override fun onMessageList(messages: List<MessageModel>?, error: String?) {
                if (messages != null) {
                    continuation.resume(messages)
                } else {
                    continuation.resumeWithException(
                        RuntimeException(error ?: "Get history failed")
                    )
                }
            }
        })
    }

    /**
     * Mark a message as read
     *
     * @param sessionId Conversation ID
     * @param messageId Message ID to mark as read
     */
    suspend fun markRead(
        sessionId: String,
        messageId: String
    ): Unit = suspendCoroutine { continuation ->
        nativeMarkRead(handle, sessionId, messageId, object : ResultCallback {
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
     * Flow of incoming messages
     *
     * Emits a new message each time one is received from the server.
     */
    val messageFlow: Flow<MessageModel> = callbackFlow {
        val callback = MessageReceivedCallback { message ->
            trySend(message)
        }
        nativeSetReceivedCallback(handle, callback)

        awaitClose {
            nativeSetReceivedCallback(handle, null)
        }
    }

    /**
     * Set a callback for incoming messages (alternative to Flow)
     *
     * @param callback Callback to invoke when a message is received, or null to clear
     */
    fun setReceivedCallback(callback: MessageReceivedCallback?) {
        nativeSetReceivedCallback(handle, callback)
    }

    // Native methods
    private external fun nativeSendText(
        handle: Long,
        sessionId: String,
        content: String,
        callback: ResultCallback
    )

    private external fun nativeGetHistory(
        handle: Long,
        sessionId: String,
        beforeTimestampMs: Long,
        limit: Int,
        callback: MessageListCallback
    )

    private external fun nativeMarkRead(
        handle: Long,
        sessionId: String,
        messageId: String,
        callback: ResultCallback
    )

    private external fun nativeSetReceivedCallback(
        handle: Long,
        callback: MessageReceivedCallback?
    )
}
