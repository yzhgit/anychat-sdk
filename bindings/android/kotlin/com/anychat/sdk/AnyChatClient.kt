package com.anychat.sdk

/**
 * Kotlin wrapper around the SWIG-generated JNI bindings.
 * Provides a coroutine-friendly API for Android developers.
 */
class AnyChatClient(private val config: ClientConfig) {

    init {
        System.loadLibrary("anychat_android")
    }

    // TODO: delegate to SWIG-generated native methods
    external fun nativeConnect(): Long
    external fun nativeDisconnect(handle: Long)
    external fun nativeGetConnectionState(handle: Long): Int

    companion object {
        // Maps native ConnectionState int to Kotlin enum
        enum class State { DISCONNECTED, CONNECTING, CONNECTED, RECONNECTING }
    }
}

data class ClientConfig(
    val gatewayUrl: String,
    val apiBaseUrl: String,
    val connectTimeoutMs: Int = 10_000,
    val autoReconnect: Boolean = true,
)
