package com.anychat.sdk

import com.anychat.sdk.models.ConnectionState
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine

/**
 * Client configuration
 */
data class ClientConfig(
    val gatewayUrl: String,
    val apiBaseUrl: String,
    val deviceId: String,
    val dbPath: String,
    val connectTimeoutMs: Int = 10_000,
    val maxReconnectAttempts: Int = 5,
    val autoReconnect: Boolean = true
)

/**
 * Main entry point for AnyChat SDK
 *
 * Usage:
 * ```
 * val config = ClientConfig(
 *     gatewayUrl = "wss://api.anychat.io",
 *     apiBaseUrl = "https://api.anychat.io/api/v1",
 *     deviceId = getDeviceId(),
 *     dbPath = context.getDatabasePath("anychat.db").absolutePath
 * )
 * val client = AnyChatClient(config)
 * client.connect()
 *
 * // Listen to connection state
 * client.connectionStateFlow.collect { state ->
 *     when (state) {
 *         ConnectionState.CONNECTED -> // Handle connected
 *         ConnectionState.DISCONNECTED -> // Handle disconnected
 *     }
 * }
 * ```
 */
class AnyChatClient(private val config: ClientConfig) {

    private var nativeHandle: Long = 0
    private var connectionCallback: ConnectionStateCallback? = null

    // Sub-modules
    val auth: Auth by lazy { Auth(nativeGetAuth(nativeHandle)) }
    val message: Message by lazy { Message(nativeGetMessage(nativeHandle)) }
    val conversation: Conversation by lazy { Conversation(nativeGetConversation(nativeHandle)) }
    val friend: Friend by lazy { Friend(nativeGetFriend(nativeHandle)) }
    val group: Group by lazy { Group(nativeGetGroup(nativeHandle)) }

    init {
        System.loadLibrary("anychat_jni")
        nativeHandle = nativeCreate(
            config.gatewayUrl,
            config.apiBaseUrl,
            config.deviceId,
            config.dbPath,
            config.connectTimeoutMs,
            config.maxReconnectAttempts,
            config.autoReconnect
        )
        if (nativeHandle == 0L) {
            throw RuntimeException("Failed to create AnyChatClient")
        }
    }

    /**
     * Connect to the server
     */
    fun connect() {
        nativeConnect(nativeHandle)
    }

    /**
     * Disconnect from the server
     */
    fun disconnect() {
        nativeDisconnect(nativeHandle)
    }

    /**
     * Get current connection state
     */
    val connectionState: ConnectionState
        get() = ConnectionState.values().find {
            it.value == nativeGetConnectionState(nativeHandle)
        } ?: ConnectionState.DISCONNECTED

    /**
     * Flow of connection state changes
     */
    val connectionStateFlow: Flow<ConnectionState> = callbackFlow {
        val callback = ConnectionStateCallback { state ->
            val connectionState = ConnectionState.values().find { it.value == state }
                ?: ConnectionState.DISCONNECTED
            trySend(connectionState)
        }
        connectionCallback = callback
        nativeSetConnectionCallback(nativeHandle, callback)

        // Send current state
        trySend(connectionState)

        awaitClose {
            nativeSetConnectionCallback(nativeHandle, null)
            connectionCallback = null
        }
    }

    /**
     * Destroy the client and release resources
     */
    fun destroy() {
        if (nativeHandle != 0L) {
            nativeDestroy(nativeHandle)
            nativeHandle = 0
        }
    }

    // Native methods
    private external fun nativeCreate(
        gatewayUrl: String,
        apiBaseUrl: String,
        deviceId: String,
        dbPath: String,
        connectTimeoutMs: Int,
        maxReconnectAttempts: Int,
        autoReconnect: Boolean
    ): Long

    private external fun nativeDestroy(handle: Long)
    private external fun nativeConnect(handle: Long)
    private external fun nativeDisconnect(handle: Long)
    private external fun nativeGetConnectionState(handle: Long): Int
    private external fun nativeSetConnectionCallback(handle: Long, callback: ConnectionStateCallback?)
    private external fun nativeGetAuth(handle: Long): Long
    private external fun nativeGetMessage(handle: Long): Long
    private external fun nativeGetConversation(handle: Long): Long
    private external fun nativeGetFriend(handle: Long): Long
    private external fun nativeGetGroup(handle: Long): Long

    companion object {
        init {
            System.loadLibrary("anychat_jni")
        }
    }
}
