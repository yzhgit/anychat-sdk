# SDK Examples Guide

This document explains the example applications included with each platform SDK.

## Overview

Each platform SDK includes an **example** application that demonstrates core functionality:

| Platform | Path | Language | Features |
|----------|------|----------|----------|
| **Flutter** | `packages/flutter/example/` | Dart | Login, Conversations, Friends, Messages |
| **Android** | `packages/android/example/` | Kotlin | All SDK features with Logcat output |
| **iOS** | `packages/ios/Example/` | Swift (SwiftUI) | Full chat app with tabs |
| **Web** | `packages/web/example/` | TypeScript | Real-time chat interface |

## Purpose

These examples are designed to:

1. **Demonstrate SDK APIs** - Show how to use core functionality
2. **Quick Testing** - Validate SDK features during development
3. **Learning Resource** - Help developers understand integration
4. **Starting Point** - Provide code to copy for new projects

## Quick Start

### Flutter

```bash
cd packages/flutter/example
flutter pub get
flutter run -d linux  # or macos, windows
```

### Android

```bash
cd packages/android
./gradlew :example:installDebug
adb logcat -s AnyChatExample  # View logs
```

### iOS

```bash
cd packages/ios/Example
pod install
open AnyChatSDKExample.xcworkspace
# Run in Xcode (Cmd+R)
```

### Web

```bash
cd packages/web/example
npm install
npm run dev
# Open http://localhost:5173
```

## Example Features

All examples demonstrate:

- ✅ SDK Initialization
- ✅ Connection Management
- ✅ User Authentication (Login/Logout)
- ✅ Real-time Messaging
- ✅ Conversation List
- ✅ Friend Management
- ✅ Error Handling

### Platform-Specific Features

**Flutter**: Desktop-focused UI with Material Design

**Android**: Comprehensive Logcat logging for all operations

**iOS**: SwiftUI with tab-based navigation

**Web**: Vanilla TypeScript with modern CSS

## Configuration

### Development vs Production

All examples are pre-configured for **local development** testing:

**Local Development** (Default):
- Uses unencrypted WebSocket (`ws://`) and HTTP (`http://`)
- Server running on local machine: `192.168.2.100:8080`
- ⚠️ **Change IP to your actual machine IP**

**Production**:
- Uses encrypted WebSocket (`wss://`) and HTTPS (`https://`)
- Points to production server
- Example: `wss://api.anychat.io` and `https://api.anychat.io/api/v1`

### 🔧 How to Configure

#### Option 1: Local Development (Current Settings)

All examples are configured for local testing at `192.168.2.100:8080`.

**Update to YOUR machine's IP**:

```bash
# Find your local IP
# Linux/Mac:
ip addr show | grep "inet " | grep -v 127.0.0.1

# Windows:
ipconfig | findstr IPv4
```

Then update each example:

**Flutter** (`packages/flutter/example/lib/main.dart`):
```dart
// Line 42-43
final _gatewayController = TextEditingController(text: 'ws://192.168.2.100:8080');
final _apiController = TextEditingController(text: 'http://192.168.2.100:8080/api/v1');
```

**Android** (`packages/android/example/.../MainActivity.kt`):
```kotlin
// Line 25-26
private const val GATEWAY_URL = "ws://192.168.2.100:8080/api/v1/ws"
private const val API_BASE_URL = "http://192.168.2.100:8080/api/v1"
```

**iOS** (`packages/ios/Example/.../ContentView.swift`):
```swift
// Line 33-34
let config = ClientConfig(
    gatewayURL: "ws://192.168.2.100:8080",
    apiBaseURL: "http://192.168.2.100:8080/api/v1",
    // ...
)
```

**Web** (`packages/web/example/src/app.ts`):
```typescript
// Line 16-17
gatewayUrl: 'ws://192.168.2.100:8080',
apiBaseUrl: 'http://192.168.2.100:8080/api/v1',
```

#### Option 2: Production Server

For production testing, change to HTTPS/WSS:

```dart
// Flutter
'wss://your-server.com'
'https://your-server.com/api/v1'
```

```kotlin
// Android
"wss://your-server.com"
"https://your-server.com/api/v1"
```

```swift
// iOS
"wss://your-server.com"
"https://your-server.com/api/v1"
```

```typescript
// Web
'wss://your-server.com'
'https://your-server.com/api/v1'
```

### 📍 127.0.0.1 vs Local IP

| Address | Use Case | Accessible From |
|---------|----------|-----------------|
| `127.0.0.1` | Single machine testing | Same machine only |
| `192.168.x.x` | LAN testing | All devices on same network |
| `0.0.0.0` | Server binding | Listen on all interfaces |

**Recommendation**: Use your local IP (`192.168.x.x`) for development to enable:
- ✅ Testing from mobile devices on same WiFi
- ✅ Multi-device testing
- ✅ More realistic testing scenario

## Testing Credentials

For local development:

```
Account: test@example.com
Password: password123
```

**Important:** Update these for production!

## Troubleshooting

### Common Issues

**Issue:** SDK not found / Module not found

**Solution:** Build the parent SDK first:

```bash
# Flutter
cd packages/flutter && flutter pub get

# Android
cd packages/android && ./gradlew assemble

# iOS
cd packages/ios && pod install

# Web
cd packages/web && npm install && npm run build
```

**Issue:** Connection timeout

**Solutions:**
1. Verify server URLs are correct
2. Check if AnyChat server is running
3. Ensure device/simulator has internet access
4. Check firewall/proxy settings

**Issue:** Build errors

**Solutions:**
1. Clean build: `flutter clean` / `./gradlew clean` / etc.
2. Update dependencies
3. Check SDK documentation for prerequisites

## Code Complexity

Examples are intentionally **simple** (~200-500 lines per platform):

- **Flutter Example**: 375 lines (single file)
- **Android Example**: 287 lines (single activity)
- **iOS Example**: 627 lines (comprehensive SwiftUI)
- **Web Example**: 383 lines (vanilla TypeScript)

For production apps, consider:
- Proper architecture (MVVM, Clean Architecture)
- State management (Provider, Redux, etc.)
- Error boundaries and user feedback
- Offline support and caching
- Push notifications
- Advanced UI/UX

## Next Steps

After running the examples:

1. **Read SDK Documentation** - See each `packages/*/README.md`
2. **Explore Code** - Understand API usage patterns
3. **Modify Examples** - Experiment with different features
4. **Build Your App** - Use examples as starting point

## Production Applications

For production-ready applications with advanced features, consider creating a separate project with:

- **Complete UI/UX** - Professional design
- **State Management** - Proper architecture
- **Testing** - Unit and integration tests
- **CI/CD** - Automated builds and deployment
- **Analytics** - User behavior tracking
- **Error Monitoring** - Crash reporting

## Support

If you encounter issues:

1. Check the example README in each platform directory
2. Review SDK documentation
3. Check [GitHub Issues](https://github.com/yzhgit/anychat-sdk/issues)
4. Contact support team

## License

All examples are provided under the same MIT license as the SDK.
