# AnyChat SDK

Multi-platform client SDK for the [AnyChat](https://github.com/yzhgit/anychat-server) instant messaging system.

Built with a C++ core and [SWIG](https://www.swig.org/)-generated bindings for each platform.

## Platform Support

| Platform | Language   | Status      | Package             |
|----------|-----------|-------------|---------------------|
| Android  | Kotlin/Java | 🚧 In Progress | Maven Central (TBD) |
| iOS      | Swift/ObjC  | 🚧 In Progress | CocoaPods / SPM (TBD) |
| Flutter  | Dart        | 🚧 In Progress | pub.dev (TBD)       |
| Web      | JavaScript  | 🚧 In Progress | npm (TBD)           |
| C++      | C++17       | 🚧 In Progress | —                   |

## Architecture

```
┌─────────────────────────────────────────────┐
│            Platform SDK (Dart/JS/Kotlin/Swift) │
├─────────────────────────────────────────────┤
│         SWIG-generated Bindings              │
├─────────────────────────────────────────────┤
│              C++ Core SDK                    │
│  (WebSocket · gRPC · Protobuf · crypto)      │
└─────────────────────────────────────────────┘
```

## Repository Structure

```
anychat-sdk/
├── core/          # C++ SDK core (headers + implementation)
├── swig/          # SWIG interface files (.i)
├── bindings/      # Platform-specific wrapper code (JNI, ObjC, Dart FFI, Emscripten)
├── packages/      # Published SDK packages (Gradle, podspec, pubspec, package.json)
├── examples/      # Sample apps per platform
├── tools/         # Build and release scripts
└── docs/          # Platform guides
```

## Prerequisites

- CMake 3.20+
- C++17 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- Python 3.7+ (`pip install pyyaml jinja2`)
- SWIG 4.1+

System dependencies:
- **libcurl** (Ubuntu: `sudo apt install libcurl4-openssl-dev`)
- **libwebsockets** (Ubuntu: `sudo apt install libwebsockets-dev`)

Per-platform:
- **Android**: Android NDK r25+, Android Studio
- **iOS**: Xcode 14+, macOS
- **Flutter**: Flutter 3.0+, Dart 3.0+
- **Web**: Emscripten 3.1+

## Building

### 1. Generate API models (required before first build)

```bash
python tools/generate-models.py
```

This generates C++ model classes from API contracts in `generated/`.

### 2. Build C++ Core

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

### 3. Generate platform bindings

```bash
# Generate all platform bindings from SWIG interface files
python tools/generate-bindings.py

# Or per platform
python tools/generate-bindings.py android
python tools/generate-bindings.py ios
python tools/generate-bindings.py flutter
python tools/generate-bindings.py web
```

### 4. Platform builds

```bash
python tools/build-android.py    # outputs packages/android/anychat-sdk.aar
python tools/build-ios.py        # outputs packages/ios/AnyChatSDK.xcframework
python tools/build-web.py        # outputs packages/web/dist/anychat.wasm + anychat.js
```

## Versioning

All platform SDKs share a single version number. Releases are triggered by the release script:

```bash
python tools/release.py 0.1.0
# Tags v0.1.0, pushes to remote, CI automatically publishes all platform packages
```

## License

MIT License — see [LICENSE](LICENSE) file.
