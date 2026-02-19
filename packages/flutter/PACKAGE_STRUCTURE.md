# AnyChat SDK Flutter Package Structure

This document describes the structure of the publishable Flutter package at `packages/flutter/`.

## Overview

The `packages/flutter/` directory contains the publishable Flutter plugin that wraps the FFI bindings from `bindings/flutter/`. This structure allows for:

1. Clear separation between internal bindings and public API
2. Publishable package ready for pub.dev
3. Proper platform-specific build configurations
4. Comprehensive documentation and examples

## Directory Structure

```
packages/flutter/
├── android/
│   └── build.gradle              # Android build config (delegates to bindings)
├── ios/
│   └── anychat_sdk.podspec       # iOS build config (delegates to bindings)
├── linux/
│   └── CMakeLists.txt            # Linux build config (delegates to bindings)
├── macos/
│   └── anychat_sdk.podspec       # macOS build config (delegates to bindings)
├── windows/
│   └── CMakeLists.txt            # Windows build config (delegates to bindings)
├── lib/
│   ├── anychat_sdk.dart          # Main library export with documentation
│   └── src/
│       ├── anychat_client.dart   # Re-exports from bindings
│       └── models.dart           # Re-exports from bindings
├── test/
│   └── anychat_sdk_test.dart     # Basic package tests
├── example/
│   ├── lib/
│   │   └── main.dart             # Example Flutter app
│   ├── pubspec.yaml              # Example app dependencies
│   ├── README.md                 # Example app documentation
│   └── .gitignore                # Example app gitignore
├── pubspec.yaml                  # Package metadata and dependencies
├── README.md                     # Main package documentation (pub.dev)
├── CHANGELOG.md                  # Version history
├── LICENSE                       # MIT License (symlink to root)
├── PUBLISHING.md                 # Publishing guide
├── analysis_options.yaml         # Dart analyzer configuration
├── .gitignore                    # Package gitignore
└── .pubignore                    # Files to exclude from pub.dev
```

## Key Files

### pubspec.yaml

Contains:
- Package metadata (name, description, version)
- Repository and homepage URLs
- Platform support configuration (FFI plugin for all platforms)
- Dependencies (ffi, path_provider)
- Topics for pub.dev discoverability

### README.md

Comprehensive documentation including:
- Feature list
- Platform support table
- Installation instructions
- Quick start guide
- Usage examples
- Architecture diagram
- Configuration guides
- Troubleshooting
- Links to resources

### CHANGELOG.md

Version history following Keep a Changelog format.

### lib/anychat_sdk.dart

Main library file with:
- Comprehensive package documentation
- Usage examples
- Re-exports of public API

### lib/src/

Internal implementation files that re-export the actual bindings from `../../bindings/flutter/lib/`.

## Platform Build Configurations

### Android (build.gradle)

- Delegates to `../../bindings/flutter/android/CMakeLists.txt`
- Builds native library via CMake
- No Java/Kotlin code needed (pure FFI plugin)

### iOS (anychat_sdk.podspec)

- Delegates to `../../bindings/flutter/ios/anychat_flutter.podspec`
- Links against static libraries from bindings
- Includes C headers from core

### macOS (anychat_sdk.podspec)

- Similar to iOS but for macOS platform
- Uses FlutterMacOS dependency

### Linux (CMakeLists.txt)

- Includes bindings CMake project
- Builds shared library (.so)
- Links against anychat_flutter_plugin

### Windows (CMakeLists.txt)

- Includes bindings CMake project
- Builds DLL
- Links against anychat_flutter_plugin

## Example App

The example app demonstrates:
- Client initialization
- Connection management
- Authentication (login/logout)
- Real-time event handling
- Conversation and friend list retrieval
- Comprehensive logging

## Relationship to Bindings

```
packages/flutter/              # Publishable package
├── lib/
│   └── src/
│       └── *.dart             # Re-exports from bindings
└── [platform]/                # Delegates to bindings build

bindings/flutter/              # Actual FFI implementation
├── lib/
│   ├── anychat.dart           # Main bindings export
│   └── src/
│       ├── anychat_client.dart        # High-level Dart wrapper
│       ├── anychat_ffi_bindings.dart  # Auto-generated FFI bindings
│       ├── models.dart                # Dart data models
│       └── native_loader.dart         # Platform-specific library loading
└── [platform]/                # Platform-specific build configs
    └── CMakeLists.txt / .podspec
```

The package wraps the bindings by:
1. Re-exporting the public API from bindings
2. Delegating platform builds to bindings
3. Providing pub.dev-ready structure and documentation

## Publishing Checklist

Before publishing to pub.dev:

- [x] pubspec.yaml complete with metadata
- [x] README.md with comprehensive documentation
- [x] CHANGELOG.md with version history
- [x] LICENSE file (MIT)
- [x] Example app with clear usage
- [x] Platform build configurations
- [x] API documentation (dartdoc comments)
- [x] .pubignore to exclude unnecessary files
- [x] analysis_options.yaml for code quality
- [x] Tests covering basic functionality

## Testing the Package

### Local Testing

Test the package locally before publishing:

```bash
cd packages/flutter
flutter pub get
flutter analyze
flutter test
```

### Dry Run

Test publishing without actually publishing:

```bash
cd packages/flutter
flutter pub publish --dry-run
```

### Testing in Example App

```bash
cd packages/flutter/example
flutter pub get
flutter run
```

## Version Management

This package follows semantic versioning:

- `0.1.x` - Initial development
- `1.0.0` - First stable release
- `1.x.x` - Stable with backwards compatibility

Update version in both:
- `pubspec.yaml`
- `CHANGELOG.md`

## Dependencies

### Runtime Dependencies

- `ffi: ^2.1.0` - Dart FFI support
- `path_provider: ^2.0.0` - Database path resolution

### Dev Dependencies

- `flutter_test` - Testing framework
- `flutter_lints: ^3.0.0` - Linting rules

## Future Enhancements

Planned improvements:

1. Complete async callback implementations
2. Enhanced error handling
3. Message encryption support
4. Media message support
5. Push notification integration
6. Comprehensive test coverage
7. Performance benchmarks
8. Additional examples
