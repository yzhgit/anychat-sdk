# Quick Start Guide

Get started with AnyChat Android SDK in 5 minutes.

## Installation

Add to your app's `build.gradle.kts`:

```kotlin
dependencies {
    implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
}
```

## Basic Usage

### 1. Initialize Client

```kotlin
import com.anychat.sdk.AnyChatClient
import com.anychat.sdk.ClientConfig

val config = ClientConfig(
    gatewayUrl = "wss://api.anychat.io",
    apiBaseUrl = "https://api.anychat.io/api/v1",
    deviceId = Settings.Secure.getString(contentResolver, Settings.Secure.ANDROID_ID),
    dbPath = getDatabasePath("anychat.db").absolutePath
)

val client = AnyChatClient(config)
```

### 2. Login

```kotlin
lifecycleScope.launch {
    val result = client.auth.login(
        username = "your_username",
        password = "your_password"
    )

    // Connect WebSocket
    client.connect()
}
```

### 3. Send Message

```kotlin
val message = client.message.send(
    conversationId = "conv_123",
    content = "Hello!",
    messageType = MessageType.TEXT
)
```

### 4. Receive Messages

```kotlin
lifecycleScope.launch {
    client.message.messageFlow.collect { message ->
        println("New: ${message.content}")
    }
}
```

## API Overview

### Modules

- `client.auth` - Authentication (register, login, logout)
- `client.message` - Messaging (send, receive, history)
- `client.conversation` - Conversation management
- `client.friend` - Friend management
- `client.group` - Group chat

### Connection State

```kotlin
client.connectionStateFlow.collect { state ->
    when (state) {
        ConnectionState.CONNECTED -> { /* ready */ }
        ConnectionState.DISCONNECTED -> { /* handle */ }
    }
}
```

## Examples

See [example/](example/) directory for a complete working app.

## Documentation

- Full docs: https://yzhgit.github.io/anychat-sdk
- API reference: https://yzhgit.github.io/anychat-server

## Support

- Issues: https://github.com/yzhgit/anychat-sdk/issues
- Discussions: https://github.com/yzhgit/anychat-sdk/discussions
