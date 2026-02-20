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

Each example requires updating server URLs:

**Flutter** (`lib/main.dart`):
```dart
final _gatewayController = TextEditingController(text: 'wss://api.anychat.io');
final _apiController = TextEditingController(text: 'https://api.anychat.io/api/v1');
```

**Android** (`MainActivity.kt`):
```kotlin
private const val GATEWAY_URL = "wss://api.anychat.io"
private const val API_BASE_URL = "https://api.anychat.io/api/v1"
```

**iOS** (`ContentView.swift`):
```swift
let config = ClientConfig(
    gatewayURL: "wss://api.anychat.io",
    apiBaseURL: "https://api.anychat.io/api/v1",
    // ...
)
```

**Web** (`src/app.ts`):
```typescript
gatewayUrl: 'wss://api.anychat.io',
apiBaseUrl: 'https://api.anychat.io/api/v1',
```

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
