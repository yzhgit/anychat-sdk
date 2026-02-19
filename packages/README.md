# AnyChat SDK Packages

This directory contains publishable packages for all supported platforms. Each package is ready for distribution through platform-specific package managers.

## Package Overview

| Package | Platform | Package Manager | Status | Installation |
|---------|----------|----------------|--------|--------------|
| **android** | Android | Maven Central | ✅ Ready | `implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")` |
| **flutter** | Flutter | pub.dev | ✅ Ready | `flutter pub add anychat_sdk` |
| **ios** | iOS/macOS | CocoaPods | ✅ Ready | `pod 'AnyChatSDK', '~> 0.1.0'` |
| **web** | Web/Node.js | npm | ✅ Ready | `npm install @anychat/sdk` |

## Architecture

```
packages/                    # Publishable packages (this directory)
    ↓
bindings/                    # Platform-specific bindings (JNI, FFI, embind, Swift)
    ↓
core/anychat_c/              # Stable C API layer
    ↓
core/anychat_core/           # C++ implementation
```

Each package in `packages/` wraps the corresponding bindings in `bindings/`, providing:
- Complete package metadata for publication
- Comprehensive documentation (README, CHANGELOG, guides)
- Working example applications
- Build configurations for package managers
- Quality assurance (tests, verification scripts)

## Package Details

### 📱 Android (`packages/android/`)

**Built with**: Gradle (Kotlin DSL) + JNI + Kotlin
**Publishes to**: Maven Central
**Features**:
- Modern Gradle Kotlin DSL configuration
- Kotlin coroutines-based async API
- Flow-based event streams
- Multi-ABI support (arm64-v8a, armeabi-v7a, x86, x86_64)
- Complete example app with UI

**Key Files**:
- `build.gradle.kts` - Main build configuration
- `README.md` - User documentation
- `PUBLISHING.md` - Publishing guide
- `example/` - Complete Android app

**Quick Start**:
```kotlin
dependencies {
    implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
}

// Usage
val client = AnyChatClient(config)
client.connect()
val token = client.login("user@example.com", "password")
```

### 🎯 Flutter (`packages/flutter/`)

**Built with**: Dart FFI
**Publishes to**: pub.dev
**Features**:
- Auto-generated FFI bindings via ffigen
- Future-based async API
- Stream-based events
- Cross-platform (Android, iOS, Linux, macOS, Windows)
- Complete SwiftUI/Material example

**Key Files**:
- `pubspec.yaml` - Package configuration
- `lib/anychat_sdk.dart` - Main export
- `README.md` - User documentation
- `PUBLISHING.md` - Publishing guide
- `example/` - Flutter demo app

**Quick Start**:
```yaml
dependencies:
  anychat_sdk: ^0.1.0
```

```dart
final client = AnyChatClient(
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1',
);
await client.connect();
await client.login(account: 'user@example.com', password: 'password');
```

### 🍎 iOS/macOS (`packages/ios/`)

**Built with**: Swift + CocoaPods
**Publishes to**: CocoaPods Trunk
**Features**:
- Swift async/await API
- AsyncStream for events
- SPM and CocoaPods support
- iOS 13.0+ and macOS 10.15+ support
- Complete SwiftUI example app

**Key Files**:
- `AnyChatSDK.podspec` - CocoaPods specification
- `README.md` - User documentation
- `PUBLISHING.md` - Publishing guide
- `Example/` - iOS/macOS example app

**Quick Start**:
```ruby
# Podfile
pod 'AnyChatSDK', '~> 0.1.0'
```

```swift
let client = try AnyChatClient(
    gatewayURL: "wss://api.anychat.io",
    apiBaseURL: "https://api.anychat.io/api/v1"
)
try await client.connect()
let token = try await client.login(account: "user@example.com", password: "password")
```

### 🌐 Web (`packages/web/`)

**Built with**: TypeScript + Emscripten + Rollup
**Publishes to**: npm
**Features**:
- Promise-based TypeScript API
- EventEmitter pattern for events
- WebAssembly (WASM) for performance
- ESM and CommonJS dual format
- Complete HTML5 example app

**Key Files**:
- `package.json` - npm package configuration
- `src/index.ts` - Main entry point
- `README.md` - User documentation
- `DEVELOPMENT.md` - Build and publish guide
- `example/` - HTML5 chat app

**Quick Start**:
```bash
npm install @anychat/sdk
```

```typescript
import { AnyChatClient } from '@anychat/sdk';

const client = new AnyChatClient({
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1'
});

await client.connect();
const token = await client.login('user@example.com', 'password');

// Listen to events
client.on('message', (message) => {
  console.log('New message:', message);
});
```

## Documentation Structure

Each package includes comprehensive documentation:

### Required Files
- **README.md** - Main package documentation with installation, usage, API reference
- **CHANGELOG.md** - Version history following [Keep a Changelog](https://keepachangelog.com/)
- **LICENSE** - MIT License (symlink or copy from root)

### Platform-Specific Guides
- **Android**: QUICKSTART.md, STRUCTURE.md, REFERENCE.md, PUBLISHING.md
- **Flutter**: PACKAGE_STRUCTURE.md, QUICK_REFERENCE.md, PUBLISHING.md
- **iOS**: INSTALL.md, IMPLEMENTATION.md, PUBLISHING.md
- **Web**: QUICKSTART.md, DEVELOPMENT.md

### Example Applications
All packages include working example applications demonstrating:
- SDK initialization
- User authentication
- Connection management
- Real-time messaging
- Event handling
- Error handling

## Building and Publishing

### Prerequisites

**All platforms**:
```bash
git submodule update --init --recursive
```

**C++ Core**:
```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
```

### Per-Platform Instructions

**Android**:
```bash
cd packages/android
./gradlew publishToMavenLocal  # Test locally
./gradlew publish              # Publish to Maven Central
```

**Flutter**:
```bash
cd packages/flutter
flutter pub publish --dry-run  # Test
flutter pub publish            # Publish
```

**iOS**:
```bash
cd packages/ios
pod spec lint AnyChatSDK.podspec       # Validate
pod trunk push AnyChatSDK.podspec      # Publish
```

**Web**:
```bash
cd packages/web
./build.sh                    # Build
npm publish --dry-run         # Test
npm publish --access public   # Publish
```

## Package Statistics

| Package | Files Created | Total Lines | Documentation | Examples |
|---------|--------------|-------------|---------------|----------|
| Android | 20+ | 3,500+ | 7 docs | Full app |
| Flutter | 22 | 2,500+ | 7 docs | Flutter app |
| iOS | 9 | 1,700+ | 5 docs | SwiftUI app |
| Web | 22 | 2,500+ | 6 docs | HTML5 app |
| **Total** | **73+** | **10,200+** | **25 docs** | **4 apps** |

## Quality Assurance

Each package includes:
- **Verification scripts**: Automated structure validation
- **Example apps**: Working demonstrations
- **Documentation**: Comprehensive guides
- **Build configs**: Ready-to-publish configurations
- **Tests**: Unit and integration tests (where applicable)

## Version Synchronization

All packages share the same version number: **0.1.0**

Version is defined in:
- `packages/android/gradle.properties` → `VERSION_NAME=0.1.0`
- `packages/flutter/pubspec.yaml` → `version: 0.1.0`
- `packages/ios/AnyChatSDK.podspec` → `s.version = '0.1.0'`
- `packages/web/package.json` → `"version": "0.1.0"`

To bump version across all packages, use:
```bash
python tools/bump_version.py 0.2.0
```

## Publishing Workflow

1. **Update CHANGELOG.md** in each package
2. **Bump version** across all packages
3. **Build and test** each package locally
4. **Create git tag**: `git tag v0.1.0 && git push --tags`
5. **Publish** to each platform's package manager:
   - Android: Maven Central (via Sonatype OSSRH)
   - Flutter: pub.dev (via `flutter pub publish`)
   - iOS: CocoaPods (via `pod trunk push`)
   - Web: npm (via `npm publish`)

## Support and Issues

- **Issues**: [GitHub Issues](https://github.com/yzhgit/anychat-sdk/issues)
- **Backend Docs**: [AnyChat Server API](https://yzhgit.github.io/anychat-server)
- **Source**: [GitHub Repository](https://github.com/yzhgit/anychat-sdk)

## License

All packages are distributed under the MIT License. See [LICENSE](../LICENSE) for details.
