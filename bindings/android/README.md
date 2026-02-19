# AnyChat Android SDK

Complete Android JNI bindings for the AnyChat IM SDK with Kotlin coroutines and Flow support.

## Features

- **Full C API Coverage**: Complete bindings for all AnyChat C APIs
- **Kotlin Coroutines**: All async operations use suspending functions
- **Reactive Streams**: Flow-based event streams for messages and updates
- **Type Safety**: Strong typing with Kotlin data classes and enums
- **Memory Safe**: Proper JNI GlobalRef management for callbacks
- **Thread Safe**: All C callbacks properly marshaled to JVM threads

## Architecture

```
bindings/android/
├── build.gradle.kts          # Android library configuration
├── CMakeLists.txt             # Native build configuration
├── src/main/
│   ├── AndroidManifest.xml
│   ├── cpp/                   # JNI bridge layer
│   │   ├── jni_bridge.cpp     # Main client JNI
│   │   ├── jni_helpers.h/cpp  # JNI utility functions
│   │   ├── jni_auth.cpp       # Auth JNI
│   │   ├── jni_message.cpp    # Message JNI
│   │   ├── jni_conversation.cpp
│   │   ├── jni_friend.cpp
│   │   └── jni_group.cpp
│   └── kotlin/com/anychat/sdk/
│       ├── AnyChatClient.kt   # Main SDK entry point
│       ├── Auth.kt            # Authentication manager
│       ├── Message.kt         # Message manager
│       ├── Conversation.kt    # Conversation manager
│       ├── Friend.kt          # Friend manager
│       ├── Group.kt           # Group manager
│       ├── Callbacks.kt       # Callback interfaces
│       └── models/
│           └── Models.kt      # Data classes
```

## Usage

### 1. Add Dependency

```gradle
dependencies {
    implementation(project(":bindings:android"))
}
```

### 2. Initialize Client

```kotlin
import com.anychat.sdk.AnyChatClient
import com.anychat.sdk.ClientConfig

val config = ClientConfig(
    gatewayUrl = "wss://api.anychat.io",
    apiBaseUrl = "https://api.anychat.io/api/v1",
    deviceId = getDeviceId(),
    dbPath = context.getDatabasePath("anychat.db").absolutePath
)

val client = AnyChatClient(config)
```

### 3. Connect and Monitor State

```kotlin
import kotlinx.coroutines.flow.collect
import com.anychat.sdk.models.ConnectionState

// Connect to server
client.connect()

// Listen to connection state changes
lifecycleScope.launch {
    client.connectionStateFlow.collect { state ->
        when (state) {
            ConnectionState.CONNECTED -> println("Connected!")
            ConnectionState.DISCONNECTED -> println("Disconnected")
            ConnectionState.CONNECTING -> println("Connecting...")
            ConnectionState.RECONNECTING -> println("Reconnecting...")
        }
    }
}
```

### 4. Authentication

```kotlin
import kotlinx.coroutines.launch

lifecycleScope.launch {
    try {
        // Login
        val token = client.auth.login(
            account = "user@example.com",
            password = "password123",
            deviceType = "android"
        )
        println("Logged in! Token: ${token.accessToken}")

        // Check login state
        if (client.auth.isLoggedIn) {
            println("User is logged in")
        }

        // Logout
        client.auth.logout()
    } catch (e: Exception) {
        println("Auth error: ${e.message}")
    }
}
```

### 5. Messaging

```kotlin
import kotlinx.coroutines.launch
import kotlinx.coroutines.flow.collect

// Send a message
lifecycleScope.launch {
    try {
        client.message.sendText(
            sessionId = "conv_123",
            content = "Hello, World!"
        )
        println("Message sent!")
    } catch (e: Exception) {
        println("Send error: ${e.message}")
    }
}

// Receive messages
lifecycleScope.launch {
    client.message.messageFlow.collect { message ->
        println("New message: ${message.content}")
        println("From: ${message.senderId}")
        println("Time: ${message.timestampMs}")
    }
}

// Get message history
lifecycleScope.launch {
    try {
        val messages = client.message.getHistory(
            sessionId = "conv_123",
            beforeTimestampMs = 0,
            limit = 20
        )
        println("Got ${messages.size} messages")
    } catch (e: Exception) {
        println("History error: ${e.message}")
    }
}
```

### 6. Conversations

```kotlin
import kotlinx.coroutines.launch

// Get conversation list
lifecycleScope.launch {
    try {
        val conversations = client.conversation.getList()
        conversations.forEach { conv ->
            println("Conv: ${conv.convId}")
            println("Unread: ${conv.unreadCount}")
            println("Last msg: ${conv.lastMsgText}")
        }
    } catch (e: Exception) {
        println("Error: ${e.message}")
    }
}

// Mark as read
lifecycleScope.launch {
    client.conversation.markRead("conv_123")
}

// Pin/unpin conversation
lifecycleScope.launch {
    client.conversation.setPinned("conv_123", pinned = true)
}

// Listen to conversation updates
lifecycleScope.launch {
    client.conversation.conversationUpdateFlow.collect { conv ->
        println("Conversation updated: ${conv.convId}")
    }
}
```

### 7. Friends

```kotlin
import kotlinx.coroutines.launch

// Get friend list
lifecycleScope.launch {
    val friends = client.friend.getList()
    friends.forEach { friend ->
        println("Friend: ${friend.userInfo.username}")
        println("Remark: ${friend.remark}")
    }
}

// Send friend request
lifecycleScope.launch {
    client.friend.sendRequest(
        toUserId = "user_456",
        message = "Let's be friends!"
    )
}

// Handle friend request
lifecycleScope.launch {
    val requests = client.friend.getPendingRequests()
    requests.forEach { request ->
        // Accept or reject
        client.friend.handleRequest(
            requestId = request.requestId,
            accept = true
        )
    }
}
```

### 8. Groups

```kotlin
import kotlinx.coroutines.launch

// Get group list
lifecycleScope.launch {
    val groups = client.group.getList()
    groups.forEach { group ->
        println("Group: ${group.name}")
        println("Members: ${group.memberCount}")
    }
}

// Create a group
lifecycleScope.launch {
    client.group.create(
        name = "My Group",
        memberIds = arrayOf("user_1", "user_2", "user_3")
    )
}

// Get group members
lifecycleScope.launch {
    val members = client.group.getMembers(
        groupId = "group_123",
        page = 1,
        pageSize = 50
    )
    members.forEach { member ->
        println("Member: ${member.userInfo.username}")
        println("Role: ${member.memberRole}")
    }
}

// Invite to group
lifecycleScope.launch {
    client.group.invite(
        groupId = "group_123",
        userIds = arrayOf("user_4", "user_5")
    )
}
```

### 9. Cleanup

```kotlin
override fun onDestroy() {
    super.onDestroy()
    client.disconnect()
    client.destroy()
}
```

## JNI Bridge Design

### Memory Management

- **GlobalRef**: Java callback objects are stored as GlobalRefs in C++ to survive GC
- **Auto-cleanup**: CallbackContext automatically deletes GlobalRefs in destructor
- **Thread safety**: All C callbacks attach to JVM and properly marshal to Java threads

### Callback Pattern

```cpp
// C callback wrapper
static void authCallback(void* userdata, int success, const AnyChatAuthToken_C* token, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    JNIEnv* env = getEnvForCallback(ctx->jvm);

    // Convert C types to Java objects
    jobject tokenObj = convertAuthToken(env, *token);

    // Call Java callback
    env->CallVoidMethod(ctx->callback, mid, success, tokenObj, errorStr);

    // Cleanup
    delete ctx;
}
```

### Type Conversion

- C strings → Java String (UTF-8)
- C structs → Kotlin data classes
- C arrays → Java ArrayList
- Proper memory cleanup with DeleteLocalRef

## Building

### Prerequisites

- Android NDK r25 or later
- CMake 3.22.1 or later
- Kotlin 1.9+

### Build Steps

```bash
# From project root
./gradlew :bindings:android:assembleDebug

# Or from Android Studio
# Build > Make Module 'bindings.android'
```

### Output

- AAR library: `bindings/android/build/outputs/aar/android-debug.aar`
- JNI libraries: `src/main/jniLibs/{abi}/libanychat_jni.so`

## Supported ABIs

- armeabi-v7a (32-bit ARM)
- arm64-v8a (64-bit ARM)
- x86 (32-bit x86)
- x86_64 (64-bit x86)

## Error Handling

All async operations use Kotlin coroutines and throw exceptions on failure:

```kotlin
lifecycleScope.launch {
    try {
        client.auth.login(account, password)
    } catch (e: RuntimeException) {
        // Handle error
        Log.e(TAG, "Login failed: ${e.message}")
    }
}
```

## Threading

- **Main thread**: Kotlin coroutines run on main/UI thread by default
- **C callbacks**: Executed on C++ worker threads, automatically attached to JVM
- **JNI bridge**: Thread-safe, properly marshals callbacks to Java threads

## License

See main project LICENSE file.
