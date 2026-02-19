# anychat_flutter

AnyChat IM SDK for Flutter — native FFI bindings for Android, iOS, Windows, Linux, and macOS.

## Features

- 🔐 **Authentication**: Login, register, token management
- 💬 **Messaging**: Send text/media, fetch history, read receipts
- 👥 **Conversations**: List, pin, mute, delete conversations
- 🤝 **Friends**: Friend requests, blacklist management
- 👪 **Groups**: Create, join, invite, manage members
- 📁 **File Transfer**: Upload/download with progress tracking
- 📞 **RTC**: Voice/video calls, meetings
- 👤 **User**: Profile, settings, search

## Architecture

This package uses Dart FFI to call the native C API (`anychat_c`) compiled from the C++ core library (`anychat_core`). This provides:

- **Cross-compiler ABI stability** (MSVC/GCC/Clang compatible)
- **Native performance** (no message serialization overhead)
- **Cross-platform** (same codebase for all platforms)

```
┌──────────────────────────────┐
│  Flutter App (Dart)          │
└──────────────────────────────┘
              ↓ FFI
┌──────────────────────────────┐
│  anychat_flutter (Dart)      │
│  - High-level API            │
│  - Stream-based events       │
│  - Future-based async ops    │
└──────────────────────────────┘
              ↓ dart:ffi
┌──────────────────────────────┐
│  anychat_c (C ABI)           │
│  - Stable C interface        │
│  - Opaque handles            │
│  - C callbacks               │
└──────────────────────────────┘
              ↓ C++
┌──────────────────────────────┐
│  anychat_core (C++)          │
│  - WebSocket, HTTP           │
│  - SQLite DB                 │
│  - Business logic            │
└──────────────────────────────┘
```

## Installation

Add to your `pubspec.yaml`:

```yaml
dependencies:
  anychat_flutter:
    path: ../bindings/flutter  # Or use a published version
```

Run:

```bash
flutter pub get
```

## Platform Setup

### Android

The native library is built automatically via CMake. Ensure `android/CMakeLists.txt` is present.

### iOS / macOS

The native library is built via CocoaPods. Ensure the `.podspec` file is present in `ios/` or `macos/`.

### Linux / Windows

The native library is built via CMake during `flutter build`.

## Usage

```dart
import 'package:anychat_flutter/anychat.dart';

void main() async {
  // 1. Create client
  final client = AnyChatClient(
    gatewayUrl: 'wss://api.anychat.io',
    apiBaseUrl: 'https://api.anychat.io/api/v1',
    deviceId: 'my-device-id', // Generate and persist a UUID
    dbPath: '/path/to/local.db',
  );

  // 2. Listen to connection state
  client.connectionStateStream.listen((state) {
    print('Connection: $state');
  });

  // 3. Connect
  client.connect();

  // 4. Login
  try {
    final token = await client.login(
      account: 'user@example.com',
      password: 'password',
    );
    print('Logged in: ${token.accessToken}');
  } catch (e) {
    print('Login failed: $e');
  }

  // 5. Send a message
  await client.sendTextMessage(
    sessionId: 'conv-abc-123',
    content: 'Hello from Flutter!',
  );

  // 6. Listen for incoming messages
  client.messageReceivedStream.listen((message) {
    print('Received: ${message.content}');
  });

  // 7. Clean up
  client.dispose();
}
```

## Generating FFI Bindings

The FFI bindings are auto-generated from the C headers using `ffigen`:

```bash
cd bindings/flutter
dart run ffigen --config ffigen.yaml
```

This creates `lib/src/anychat_ffi_bindings.dart` from the C headers in `core/include/anychat_c/`.

## Building for Platforms

### Android

```bash
flutter build apk
# or
flutter build appbundle
```

### iOS

```bash
flutter build ios
```

### Desktop (Linux / macOS / Windows)

```bash
flutter build linux
flutter build macos
flutter build windows
```

## Known Limitations

- **Async callbacks**: The current implementation uses placeholders for async operations. Full implementation requires `NativeCallable` (Dart 2.18+) to convert Dart closures into C function pointers.
- **Memory management**: Caller must ensure proper disposal by calling `client.dispose()`.

## Development

### Project Structure

```
bindings/flutter/
├── lib/
│   ├── src/
│   │   ├── anychat_ffi_bindings.dart   # Auto-generated FFI bindings
│   │   ├── anychat_client.dart          # High-level Dart API
│   │   ├── models.dart                  # Dart data models
│   │   └── native_loader.dart           # DynamicLibrary loader
│   └── anychat.dart                     # Main export
├── android/
│   └── CMakeLists.txt                   # Android build config
├── ios/
│   └── anychat_flutter.podspec          # iOS build config
├── linux/
│   └── CMakeLists.txt                   # Linux build config
├── macos/
│   └── anychat_flutter.podspec          # macOS build config
├── windows/
│   └── CMakeLists.txt                   # Windows build config
├── example/                             # Example app
├── test/                                # Unit tests
├── pubspec.yaml                         # Package metadata
└── ffigen.yaml                          # FFI code generation config
```

## License

MIT

## Links

- [Backend API Docs](https://yzhgit.github.io/anychat-server)
- [Backend Repository](https://github.com/yzhgit/anychat-server)
- [C API Guide](../../docs/c_api_guide.md)
