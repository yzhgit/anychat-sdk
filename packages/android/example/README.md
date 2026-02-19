# AnyChat Android SDK Example

Simple example app demonstrating how to use the AnyChat Android SDK.

## Features Demonstrated

1. **Authentication**
   - User registration
   - Login/logout
   - Token management

2. **Friend Management**
   - Search for users
   - Send friend requests
   - View friend list

3. **Messaging**
   - Send text messages
   - Receive real-time messages
   - View message history
   - List conversations

4. **Group Chat**
   - Create groups
   - Add members
   - Send group messages
   - List group members

5. **Connection Management**
   - WebSocket connection status
   - Auto-reconnection
   - Connection state monitoring

## Running the Example

### Prerequisites

- Android Studio Arctic Fox or later
- Android SDK 24+ (Android 7.0)
- A running instance of [AnyChat Server](https://github.com/yzhgit/anychat-server)

### Setup

1. Clone the repository:
   ```bash
   git clone https://github.com/yzhgit/anychat-sdk.git
   cd anychat-sdk/packages/android/example
   ```

2. Open in Android Studio:
   ```
   File -> Open -> Select packages/android directory
   ```

3. Update server URLs in `MainActivity.kt`:
   ```kotlin
   private const val GATEWAY_URL = "wss://your-server.com"
   private const val API_BASE_URL = "https://your-server.com/api/v1"
   ```

4. Build and run:
   ```
   Run -> Run 'app'
   ```

### Using the Published SDK

To use the published SDK instead of the local project:

1. Edit `example/build.gradle.kts`:
   ```kotlin
   dependencies {
       // Comment out the project dependency
       // implementation(project(":"))

       // Use the published version
       implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
   }
   ```

## Code Walkthrough

### 1. Initialize the SDK

```kotlin
val config = ClientConfig(
    gatewayUrl = "wss://api.anychat.io",
    apiBaseUrl = "https://api.anychat.io/api/v1",
    deviceId = getDeviceId(),
    dbPath = getDatabasePath("anychat.db").absolutePath
)

val client = AnyChatClient(config)
```

### 2. Login and Connect

```kotlin
lifecycleScope.launch {
    val loginResult = client.auth.login(
        username = "demo_user",
        password = "password"
    )

    // Connect WebSocket after login
    client.connect()
}
```

### 3. Monitor Connection State

```kotlin
lifecycleScope.launch {
    client.connectionStateFlow.collect { state ->
        when (state) {
            ConnectionState.CONNECTED -> println("Connected!")
            ConnectionState.DISCONNECTED -> println("Disconnected")
            // ... handle other states
        }
    }
}
```

### 4. Send Messages

```kotlin
val message = client.message.send(
    conversationId = "conv_123",
    content = "Hello!",
    messageType = MessageType.TEXT
)
```

### 5. Receive Messages

```kotlin
lifecycleScope.launch {
    client.message.messageFlow.collect { message ->
        println("New message: ${message.content}")
    }
}
```

## Viewing Logs

The example app logs all operations to Logcat with tag `AnyChatExample`.

To filter logs in Android Studio:
```
View -> Tool Windows -> Logcat
Filter: package:com.anychat.example tag:AnyChatExample
```

## Troubleshooting

### Connection Issues

- Verify your server is running and accessible
- Check if the URLs are correct (no trailing slashes)
- Ensure `INTERNET` and `ACCESS_NETWORK_STATE` permissions are granted

### Build Errors

- Clean and rebuild: `Build -> Clean Project` then `Build -> Rebuild Project`
- Invalidate caches: `File -> Invalidate Caches / Restart`
- Check that NDK is installed: `Tools -> SDK Manager -> SDK Tools -> NDK`

### Runtime Crashes

- Check if native libraries are included: Look for `lib/*/libanychat_jni.so` in APK
- Verify ABI filters match your device (see `build.gradle.kts`)
- Enable detailed logging by setting log level

## Project Structure

```
example/
├── build.gradle.kts           # Gradle build script
├── src/main/
│   ├── AndroidManifest.xml    # App manifest
│   ├── java/com/anychat/example/
│   │   └── MainActivity.kt    # Main activity with examples
│   └── res/
│       ├── layout/
│       │   └── activity_main.xml
│       └── values/
│           └── strings.xml
└── README.md                  # This file
```

## Next Steps

- Customize the UI with your own design
- Add error handling and retry logic
- Implement push notifications
- Add file upload/download
- Implement message encryption

## License

This example is part of the AnyChat SDK and is released under the MIT License.
