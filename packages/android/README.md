# AnyChat Android SDK

Official Android SDK for the [AnyChat](https://github.com/yzhgit/anychat-server) instant messaging system.

[![Maven Central](https://img.shields.io/maven-central/v/io.github.yzhgit/anychat-sdk-android)](https://central.sonatype.com/artifact/io.github.yzhgit/anychat-sdk-android)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](../../LICENSE)
[![API](https://img.shields.io/badge/API-24%2B-brightgreen.svg?style=flat)](https://android-arsenal.com/api?level=24)

## Features

- **Real-time messaging** via WebSocket
- **RESTful API integration** for user management, friends, groups, and conversations
- **Local database caching** with SQLite
- **Automatic reconnection** with exponential backoff
- **Coroutine-based async API** for idiomatic Kotlin usage
- **Multi-ABI support** (arm64-v8a, armeabi-v7a, x86, x86_64)

## Installation

### Gradle (Kotlin DSL)

Add the dependency to your app's `build.gradle.kts`:

```kotlin
dependencies {
    implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
}
```

### Gradle (Groovy)

```groovy
dependencies {
    implementation 'io.github.yzhgit:anychat-sdk-android:0.1.0'
}
```

## Quick Start

### 1. Initialize the Client

```kotlin
import com.anychat.sdk.AnyChatClient
import com.anychat.sdk.ClientConfig

class MainActivity : AppCompatActivity() {

    private lateinit var client: AnyChatClient

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val config = ClientConfig(
            gatewayUrl = "wss://api.anychat.io",
            apiBaseUrl = "https://api.anychat.io/api/v1",
            deviceId = getDeviceId(),
            dbPath = getDatabasePath("anychat.db").absolutePath
        )

        client = AnyChatClient(config)
    }

    private fun getDeviceId(): String {
        // Use Android ID or generate a UUID
        return Settings.Secure.getString(
            contentResolver,
            Settings.Secure.ANDROID_ID
        )
    }
}
```

### 2. Authentication

```kotlin
import kotlinx.coroutines.launch

lifecycleScope.launch {
    try {
        // Register a new user
        val registerResult = client.auth.register(
            username = "john_doe",
            password = "secure_password",
            email = "john@example.com"
        )
        Log.d("AnyChat", "User registered: ${registerResult.user.id}")

        // Login
        val loginResult = client.auth.login(
            username = "john_doe",
            password = "secure_password"
        )
        Log.d("AnyChat", "Login successful, token: ${loginResult.token}")

        // Connect to WebSocket
        client.connect()

    } catch (e: Exception) {
        Log.e("AnyChat", "Auth failed", e)
    }
}
```

### 3. Listen to Connection State

```kotlin
lifecycleScope.launch {
    client.connectionStateFlow.collect { state ->
        when (state) {
            ConnectionState.CONNECTED -> {
                Log.d("AnyChat", "WebSocket connected")
            }
            ConnectionState.CONNECTING -> {
                Log.d("AnyChat", "Connecting...")
            }
            ConnectionState.DISCONNECTED -> {
                Log.w("AnyChat", "Disconnected")
            }
            ConnectionState.FAILED -> {
                Log.e("AnyChat", "Connection failed")
            }
        }
    }
}
```

### 4. Send and Receive Messages

```kotlin
// Send a text message
lifecycleScope.launch {
    val message = client.message.send(
        conversationId = "conv_123",
        content = "Hello, World!",
        messageType = MessageType.TEXT
    )
    Log.d("AnyChat", "Message sent: ${message.id}")
}

// Listen for new messages
lifecycleScope.launch {
    client.message.messageFlow.collect { message ->
        Log.d("AnyChat", "New message: ${message.content}")
        // Update UI
    }
}
```

### 5. Manage Friends

```kotlin
// Search for users
val users = client.friend.searchUsers(query = "john")

// Send friend request
client.friend.sendRequest(userId = "user_456")

// Get friend list
val friends = client.friend.getFriendList()
```

### 6. Group Chat

```kotlin
// Create a group
val group = client.group.create(
    name = "Dev Team",
    description = "Development team chat"
)

// Add members
client.group.addMember(
    groupId = group.id,
    userIds = listOf("user_123", "user_456")
)

// Send group message
client.message.send(
    conversationId = group.conversationId,
    content = "Team meeting at 3 PM",
    messageType = MessageType.TEXT
)
```

## Permissions

The SDK requires the following permissions (automatically included):

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```

## ProGuard

If you use ProGuard/R8, the SDK's ProGuard rules are automatically applied. No additional configuration needed.

## Architecture

The SDK is built with:

- **C++ Core Library** for business logic and networking
- **C API Layer** for cross-platform stability
- **JNI Bindings** for Android integration
- **Kotlin Coroutines** for async operations
- **SQLite** for local caching

```
┌────────────────────────────────┐
│  Kotlin API (AnyChatClient)   │
│  - Coroutines                  │
│  - Flow-based events           │
├────────────────────────────────┤
│  JNI Bindings                  │
│  - Native method calls         │
│  - Callback bridging           │
├────────────────────────────────┤
│  C API Layer                   │
│  - Stable C ABI                │
│  - Opaque handles              │
├────────────────────────────────┤
│  C++ Core SDK                  │
│  - WebSocket (libwebsockets)   │
│  - HTTP (libcurl)              │
│  - SQLite database             │
└────────────────────────────────┘
```

## API Documentation

Full API documentation is available at: https://yzhgit.github.io/anychat-sdk

Backend API: https://yzhgit.github.io/anychat-server

## Example App

See the [example directory](example/) for a complete Android app demonstrating all SDK features.

## Requirements

- Android SDK 24+ (Android 7.0 Nougat)
- Kotlin 1.9+
- AndroidX

## Supported ABIs

- `arm64-v8a` (64-bit ARM)
- `armeabi-v7a` (32-bit ARM)
- `x86_64` (64-bit Intel/AMD)
- `x86` (32-bit Intel/AMD)

## License

MIT License - see [LICENSE](../../LICENSE) file for details.

## Related Projects

- [AnyChat Server](https://github.com/yzhgit/anychat-server) - Backend implementation
- [AnyChat iOS SDK](../ios/) - iOS/macOS SDK
- [AnyChat Flutter SDK](../flutter/) - Flutter SDK

## Support

- Issues: [GitHub Issues](https://github.com/yzhgit/anychat-sdk/issues)
- Discussions: [GitHub Discussions](https://github.com/yzhgit/anychat-sdk/discussions)

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for version history.
