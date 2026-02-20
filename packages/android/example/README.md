# AnyChat Android SDK Example

A simple example app demonstrating the AnyChat Android SDK integration.

## Features Demonstrated

This example showcases:

1. **SDK Initialization** - Configure and initialize the client
2. **Connection Management** - WebSocket connection with automatic reconnection
3. **User Authentication** - Registration and login
4. **Friend Management** - Search users, send requests, view friend list
5. **Messaging** - Send and receive messages, view history
6. **Group Chat** - Create groups, add members, send group messages

## Prerequisites

1. **Android Studio** Arctic Fox or later
2. **Android SDK** API level 24+ (Android 7.0+)
3. **Kotlin** 1.8+
4. **AnyChat Server** running and accessible

## Project Structure

```
example/
├── src/main/
│   ├── java/com/anychat/example/
│   │   └── MainActivity.kt          # Main example code
│   ├── res/
│   │   ├── layout/
│   │   │   └── activity_main.xml    # Simple UI layout
│   │   └── values/
│   │       └── strings.xml          # String resources
│   └── AndroidManifest.xml          # App configuration
└── build.gradle.kts                 # Dependencies
```

## Configuration

Edit `MainActivity.kt` to point to your AnyChat server:

```kotlin
companion object {
    private const val GATEWAY_URL = "wss://your-server.com"
    private const val API_BASE_URL = "https://your-server.com/api/v1"
}
```

## Running the Example

### Option 1: Android Studio

1. Open the `packages/android` directory in Android Studio
2. Wait for Gradle sync to complete
3. Connect an Android device or start an emulator
4. Run the `example` module

### Option 2: Command Line

```bash
cd packages/android
./gradlew :example:installDebug
adb shell am start -n com.anychat.example/.MainActivity
```

## Viewing Output

All SDK operations are logged to **Android Logcat**:

```bash
# Filter by tag
adb logcat -s AnyChatExample

# Or in Android Studio:
# View → Tool Windows → Logcat
# Filter: "AnyChatExample"
```

You'll see detailed logs for:
- ✅ Registration/Login results
- 🔄 Connection state changes
- 📩 Messages sent/received
- 👥 Friend operations
- 🏘️ Group operations

## Example Output

```
D/AnyChatExample: AnyChat SDK initialized
D/AnyChatExample: 🔄 Connecting to WebSocket...
D/AnyChatExample: ✅ WebSocket connected
D/AnyChatExample: === Example 1: Authentication ===
D/AnyChatExample: ✅ Login successful
D/AnyChatExample:    User ID: 123456
D/AnyChatExample:    Token: eyJhbGciOiJIUzI1Ni...
D/AnyChatExample: === Example 2: Friend Management ===
D/AnyChatExample: ✅ Found 5 users matching 'john'
D/AnyChatExample:    - john_doe (John Doe)
D/AnyChatExample: === Example 3: Messaging ===
D/AnyChatExample: ✅ Message sent: msg_789
D/AnyChatExample: 📩 New message: Hello from AnyChat!
```

## Code Highlights

### SDK Initialization

```kotlin
val config = ClientConfig(
    gatewayUrl = GATEWAY_URL,
    apiBaseUrl = API_BASE_URL,
    deviceId = getDeviceId(),
    dbPath = getDatabasePath("anychat.db").absolutePath,
    autoReconnect = true
)

client = AnyChatClient(config)
```

### Authentication

```kotlin
// Login
val loginResult = client.auth.login(
    username = "demo_user",
    password = "SecurePassword123!"
)

// Connect WebSocket after login
client.connect()
```

### Send Message

```kotlin
val message = client.message.send(
    conversationId = convId,
    content = "Hello from Android!",
    messageType = MessageType.TEXT
)
```

### Listen for Messages

```kotlin
lifecycleScope.launch {
    client.message.messageFlow.collect { message ->
        Log.d(TAG, "📩 New message: ${message.content}")
    }
}
```

## Dependencies

The example uses:

- **AnyChat SDK** - IM functionality
- **Kotlin Coroutines** - Async operations
- **AndroidX Lifecycle** - Lifecycle-aware components
- **Material Components** - UI elements

See `build.gradle.kts` for complete dependency list.

## Troubleshooting

### Build Errors

**Issue:** Missing AnyChat SDK dependency

**Solution:**
```bash
# Build the parent SDK first
cd packages/android
./gradlew assemble
```

### Runtime Errors

**Issue:** `UnknownHostException` or connection timeout

**Solutions:**
1. Verify server URLs in `MainActivity.kt`
2. Check if AnyChat server is running
3. Ensure device has internet access
4. Add network security config if using HTTP (not recommended for production)

**Issue:** `SecurityException: Permission denied`

**Solution:** The app requires `INTERNET` permission (already added in `AndroidManifest.xml`)

### Logcat Issues

**Issue:** No logs appearing

**Solutions:**
- Ensure app is running on device/emulator
- Check Logcat filter is set to "AnyChatExample"
- Verify log level is set to "Debug" or "Verbose"

## Next Steps

This example is intentionally simple to demonstrate basic SDK usage. For a production app, consider:

- **Error Handling** - Add try/catch blocks and user-friendly error messages
- **UI/UX** - Build proper screens with Material Design
- **State Management** - Use ViewModel + StateFlow/LiveData
- **Persistence** - Save user preferences with SharedPreferences or DataStore
- **Notifications** - Implement FCM for push notifications
- **Background Sync** - Use WorkManager for syncing messages

## License

MIT License - see [../../LICENSE](../../LICENSE)

## Links

- **SDK Documentation**: [packages/android/README.md](../README.md)
- **Backend API**: https://github.com/yzhgit/anychat-server
- **Issues**: https://github.com/yzhgit/anychat-sdk/issues
