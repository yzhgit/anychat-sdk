# AnyChat Android Package - Structure Overview

This document provides a complete overview of the `packages/android/` directory structure.

## Directory Structure

```
packages/android/
├── src/
│   └── main/
│       └── AndroidManifest.xml          # Library manifest (permissions)
│
├── example/                              # Example Android app
│   ├── src/
│   │   └── main/
│   │       ├── java/com/anychat/example/
│   │       │   └── MainActivity.kt      # Example implementation
│   │       ├── res/
│   │       │   ├── layout/
│   │       │   │   └── activity_main.xml
│   │       │   └── values/
│   │       │       └── strings.xml
│   │       └── AndroidManifest.xml      # Example app manifest
│   ├── build.gradle.kts                 # Example app build config
│   ├── proguard-rules.pro               # Example ProGuard rules
│   └── README.md                        # Example documentation
│
├── gradle/
│   └── wrapper/
│       └── gradle-wrapper.properties    # Gradle wrapper configuration
│
├── build.gradle.kts                     # Main library build script
├── settings.gradle.kts                  # Gradle project settings
├── gradle.properties                    # Gradle properties (version, publishing)
├── proguard-rules.pro                   # Library ProGuard rules
│
├── README.md                            # Package documentation
├── CHANGELOG.md                         # Version history
├── QUICKSTART.md                        # Quick start guide
├── PUBLISHING.md                        # Maven Central publishing guide
├── .gitignore                           # Git ignore rules
└── verify.sh                            # Structure verification script
```

## Key Features

### 1. Gradle Configuration (Kotlin DSL)

**`build.gradle.kts`** - Modern Kotlin DSL with:
- Android library plugin configuration
- Native build setup (CMake)
- Source sets pointing to `../../bindings/android/`
- Maven publishing configuration
- POM metadata for Maven Central
- Signing configuration (commented out)

**`settings.gradle.kts`** - Project settings:
- Root project name
- Repository configuration
- Plugin management

**`gradle.properties`** - Build properties:
- Version management
- Android/Kotlin settings
- Publishing credentials (template)

### 2. Source Integration

The package **does not duplicate** the bindings code. Instead, it references the actual implementation:

```kotlin
sourceSets {
    getByName("main") {
        kotlin.srcDirs("../../bindings/android/src/main/kotlin")
        jniLibs.srcDirs("../../bindings/android/src/main/jniLibs")
    }
}
```

**Bindings location**: `../../bindings/android/src/main/kotlin/com/anychat/sdk/`

Contains:
- `AnyChatClient.kt` - Main SDK client
- `Auth.kt` - Authentication module
- `Message.kt` - Messaging module
- `Conversation.kt` - Conversation management
- `Friend.kt` - Friend management
- `Group.kt` - Group management
- `Callbacks.kt` - Event callbacks
- `models/Models.kt` - Data models

### 3. Native Build Configuration

The package uses the root CMakeLists.txt for native library compilation:

```kotlin
externalNativeBuild {
    cmake {
        path = file("../../CMakeLists.txt")
        version = "3.22.1"
    }
}
```

**Supported ABIs**:
- `arm64-v8a` (64-bit ARM)
- `armeabi-v7a` (32-bit ARM)
- `x86_64` (64-bit Intel)
- `x86` (32-bit Intel)

### 4. Maven Publishing

Configured for Maven Central with:

**GroupId**: `io.github.yzhgit`
**ArtifactId**: `anychat-sdk-android`
**Version**: From `gradle.properties`

**Generated artifacts**:
- `.aar` - Android Archive with compiled code and native libs
- `-sources.jar` - Source code for IDE integration
- `-javadoc.jar` - API documentation
- `.pom` - Maven metadata
- `.asc` files - GPG signatures (when signing enabled)

**Repository**: Sonatype OSSRH (Maven Central)

### 5. Example Application

Complete Android app demonstrating:
- SDK initialization
- User authentication (register/login)
- Friend management (search, add, list)
- Messaging (send, receive, history)
- Group chat (create, add members, send messages)
- WebSocket connection monitoring

**Project dependency** for local development:
```kotlin
implementation(project(":"))
```

**Published dependency** for end users:
```kotlin
implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
```

### 6. Documentation

**README.md**: Comprehensive package documentation
- Features overview
- Installation instructions
- Usage examples
- API documentation links
- Architecture diagram
- Requirements and support info

**QUICKSTART.md**: 5-minute quick start guide
- Basic setup
- Common operations
- Code snippets

**CHANGELOG.md**: Version history
- Release notes
- Breaking changes
- New features
- Bug fixes

**PUBLISHING.md**: Maven Central publishing guide
- Prerequisites (Sonatype account, GPG key)
- Configuration steps
- Publishing workflow
- CI/CD automation
- Troubleshooting

### 7. ProGuard Rules

**`proguard-rules.pro`** automatically keeps:
- Native methods
- AnyChat SDK public API
- Kotlin coroutines internals
- Data model classes

Applied automatically to consuming apps via `consumerProguardFiles`.

### 8. Verification Script

**`verify.sh`** - Bash script to validate:
- All required files present
- Bindings source directory exists
- Example app structure
- CMakeLists.txt presence
- Package metadata correctness

Run with:
```bash
./verify.sh
```

## Usage Scenarios

### Local Development

1. Clone repository with submodules:
   ```bash
   git clone --recurse-submodules https://github.com/yzhgit/anychat-sdk.git
   ```

2. Open in Android Studio:
   ```
   File -> Open -> packages/android
   ```

3. Build library:
   ```bash
   ./gradlew assembleRelease
   ```

4. Run example:
   ```bash
   ./gradlew :example:installDebug
   ```

### Publishing to Maven Local

Test the package locally before publishing:

```bash
./gradlew publishToMavenLocal
```

Then in another project:
```kotlin
repositories {
    mavenLocal()
}
dependencies {
    implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
}
```

### Publishing to Maven Central

See `PUBLISHING.md` for complete instructions.

Short version:
1. Configure credentials in `~/.gradle/gradle.properties`
2. Enable signing in `build.gradle.kts`
3. Run `./gradlew publish`
4. Login to Sonatype, close and release

## Dependencies

### Runtime Dependencies

Automatically included in the AAR:
- `androidx.core:core-ktx:1.12.0`
- `org.jetbrains.kotlinx:kotlinx-coroutines-core:1.7.3`
- `org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3`

### Native Libraries

Bundled in AAR as `.so` files:
- `libanychat_jni.so` - JNI bindings
- `libanychat_core.so` - C++ core SDK (optional, may be statically linked)
- `libc++_shared.so` - C++ runtime

### Build-time Dependencies

Required for building:
- Android Gradle Plugin 8.1+
- Kotlin 1.9+
- NDK 25.2+
- CMake 3.22+

## Related Files

**Outside this directory**:
- `../../bindings/android/` - Actual Kotlin/JNI implementation
- `../../CMakeLists.txt` - Root CMake configuration
- `../../core/` - C++ core SDK
- `../../thirdparty/` - Third-party dependencies (submodules)

## Maintenance Notes

### Updating Version

1. Edit `gradle.properties`:
   ```properties
   VERSION_NAME=0.2.0
   ```

2. Update `CHANGELOG.md` with release notes

3. Commit and tag:
   ```bash
   git commit -am "Release v0.2.0"
   git tag -a v0.2.0 -m "Release version 0.2.0"
   git push origin main --tags
   ```

### Adding New Features

The actual SDK code is in `../../bindings/android/src/main/kotlin/`. This package only provides the Gradle wrapper for publishing.

To add features:
1. Modify code in `bindings/android/`
2. Test with example app
3. Update documentation
4. Bump version
5. Publish

### Troubleshooting

Common issues:

**Native library not found**:
- Check NDK installation
- Verify CMake paths
- Ensure submodules are initialized

**Gradle sync failed**:
- Update Gradle wrapper: `./gradlew wrapper --gradle-version 8.4`
- Invalidate caches in Android Studio

**Publishing failed**:
- See `PUBLISHING.md` troubleshooting section
- Verify credentials
- Check signing configuration

## Support

- Issues: https://github.com/yzhgit/anychat-sdk/issues
- Discussions: https://github.com/yzhgit/anychat-sdk/discussions
- Documentation: https://yzhgit.github.io/anychat-sdk
