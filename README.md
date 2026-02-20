# AnyChat SDK

Multi-platform client SDK for the [AnyChat](https://github.com/yzhgit/anychat-server) instant messaging system.

Built with a **C++ core library** and a **stable C API layer** for cross-platform bindings.

## Platform Support

| Platform | Technology   | Status      | Package             |
|----------|-------------|-------------|---------------------|
| Android  | JNI + C API | 🚧 In Progress | Maven Central (TBD) |
| iOS      | Swift + C API | 🚧 In Progress | CocoaPods / SPM (TBD) |
| macOS    | Swift + C API | 🚧 In Progress | CocoaPods / SPM (TBD) |
| Linux    | C API       | 🚧 In Progress | —                   |
| Windows  | C API       | 🚧 In Progress | —                   |
| Flutter  | Dart FFI    | ✅ Available   | pub.dev (TBD)       |
| Web      | Emscripten  | 🚧 In Progress | npm (TBD)           |

## Architecture

```
┌─────────────────────────────────────────────┐
│  Platform SDK (Dart/Java/Swift/JS/C/C++)   │
│  - High-level, idiomatic API                │
│  - Future/Promise-based async               │
│  - Reactive streams/callbacks               │
├─────────────────────────────────────────────┤
│  Platform Binding Layer                     │
│  - JNI (Android)                            │
│  - Objective-C/Swift bridging (iOS/macOS)   │
│  - Dart FFI (Flutter)                       │
│  - Emscripten (Web)                         │
├─────────────────────────────────────────────┤
│  C API Layer (anychat_c)                    │
│  - Stable C ABI (cross-compiler compatible) │
│  - Opaque handles + C callbacks             │
│  - Error codes + TLS error messages         │
├─────────────────────────────────────────────┤
│  C++ Core SDK (anychat_core)                │
│  - WebSocket client (libwebsockets)         │
│  - HTTP client (libcurl)                    │
│  - SQLite database (local cache)            │
│  - Business logic & state management        │
└─────────────────────────────────────────────┘
```

**Why a C API layer?**

- **Cross-compiler ABI stability**: C++ ABI differs between MSVC, GCC, and Clang — even between versions of the same compiler. C ABI is standardized and stable.
- **Industry standard**: SQLite, OpenSSL, FFmpeg, and other widely-used libraries use C APIs.
- **Simpler bindings**: C types map directly to FFI types in all languages.
- **No SWIG needed**: Each platform uses native binding tools (JNI, FFI, bridging headers).

## Repository Structure

```
anychat-sdk/
├── core/                 # C++ SDK core + C API wrapper
│   ├── include/
│   │   ├── anychat/      # C++ headers (internal)
│   │   └── anychat_c/    # C headers (public API)
│   ├── src/              # C++ implementation
│   │   └── c_api/        # C wrapper implementation
│   └── tests/            # Unit tests
├── packages/             # Platform-specific SDK packages
│   ├── android/          # JNI bindings (Kotlin/Java)
│   ├── ios/              # Swift bindings
│   ├── flutter/          # Dart FFI bindings ✅
│   └── web/              # Emscripten bindings
├── thirdparty/           # Git submodules (curl, libwebsockets, etc.)
├── tools/                # Build and release scripts
└── docs/                 # Platform guides + API documentation
    └── c_api_guide.md    # C API usage guide ✅
```

## Prerequisites

### Core SDK

- **CMake** 3.20+
- **C++17 compiler**: GCC 10+, Clang 12+, or MSVC 2019+
- **Git** (for submodules)

Third-party dependencies are included as Git submodules — no system packages required:

```bash
git submodule update --init --recursive
```

### Platform-specific

- **Android**: Android NDK r25+, Android Studio
- **iOS/macOS**: Xcode 14+
- **Flutter**: Flutter 3.0+, Dart 3.0+
- **Web**: Emscripten 3.1+

## Quick Start

### 1. Clone and initialize submodules

```bash
git clone https://github.com/yzhgit/anychat-sdk.git
cd anychat-sdk
git submodule update --init --recursive
```

### 2. Build the C++ core + C API

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build  # Run unit tests
```

This builds:
- `anychat_core` (C++ static library)
- `anychat_c` (C wrapper static library)

### 3. Build platform SDKs

#### Flutter (Dart FFI)

```bash
cd packages/flutter

# Generate FFI bindings from C headers
dart run ffigen --config ffigen.yaml

# Run example app
cd example
flutter run
```

See [packages/flutter/README.md](packages/flutter/README.md) for details.

#### Android (JNI)

```bash
cd packages/android
./gradlew assembleRelease
# Outputs: build/outputs/aar/anychat-android.aar
```

*(Coming soon)*

#### iOS/macOS (Swift)

```bash
cd packages/ios
pod install
open AnyChatSDK.xcworkspace
# Build the framework
```

*(Coming soon)*

#### Web (Emscripten)

```bash
cd packages/web
emcmake cmake -B build
cmake --build build
# Outputs: build/anychat.wasm, build/anychat.js
```

*(Coming soon)*

## C API Example

```c
#include <anychat_c/anychat_c.h>

void on_login(void* userdata, int success, const AnyChatAuthToken_C* token, const char* error) {
    if (success) {
        printf("Logged in: %s\n", token->access_token);
    } else {
        printf("Login failed: %s\n", error);
    }
}

int main() {
    // Configure client
    AnyChatClientConfig_C config = {
        .gateway_url = "wss://api.anychat.io",
        .api_base_url = "https://api.anychat.io/api/v1",
        .device_id = "my-device-001",
        .db_path = "./anychat.db",
    };

    // Create client
    AnyChatClientHandle client = anychat_client_create(&config);
    anychat_client_connect(client);

    // Login
    AnyChatAuthHandle auth = anychat_client_get_auth(client);
    anychat_auth_login(auth, "user@example.com", "password", "desktop", NULL, on_login);

    // ... use the SDK ...

    // Cleanup
    anychat_client_disconnect(client);
    anychat_client_destroy(client);
    return 0;
}
```

## Documentation

- **[C API Guide](docs/c_api_guide.md)** — Memory management, callbacks, error handling
- **[Flutter SDK Guide](packages/flutter/README.md)** — Dart FFI usage
- **[Backend API Docs](https://yzhgit.github.io/anychat-server)** — Server API reference

## Development Workflow

### Adding a new feature

1. **Update C++ core**: Add methods to `core/include/anychat/*.h` and implement in `core/src/`
2. **Update C wrapper**: Add C functions to `core/include/anychat_c/*.h` and implement in `core/src/c_api/`
3. **Regenerate bindings**:
   - Flutter: `cd packages/flutter && dart run ffigen`
   - Android: Update JNI wrappers in `packages/android/src/main/cpp/`
   - iOS: Update Swift wrappers in `packages/ios/Sources/`
4. **Test**: Run platform-specific tests

### Testing

```bash
# C++ unit tests
cd build && ctest

# C API example
./build/bin/c_example

# Flutter tests
cd packages/flutter && flutter test

# Android tests
cd packages/android && ./gradlew test

# iOS tests
cd packages/ios && xcodebuild test
```

### Memory leak detection

```bash
valgrind --leak-check=full ./build/bin/c_example
```

## Versioning

All platform SDKs share a single version number defined in the root `CMakeLists.txt`.

```bash
# Bump version (updates CMakeLists.txt, pubspec.yaml, etc.)
python tools/release.py 0.2.0

# Tag and push
git tag v0.2.0
git push origin main --tags
```

CI automatically publishes all platform packages on tag push.

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make your changes
4. Run tests (`ctest`, `flutter test`, etc.)
5. Commit with conventional commits (`feat:`, `fix:`, `docs:`)
6. Push and open a pull request

## License

MIT License — see [LICENSE](LICENSE) file.

## Links

- **Backend**: [anychat-server](https://github.com/yzhgit/anychat-server)
- **API Docs**: [https://yzhgit.github.io/anychat-server](https://yzhgit.github.io/anychat-server)
- **Issues**: [GitHub Issues](https://github.com/yzhgit/anychat-sdk/issues)
