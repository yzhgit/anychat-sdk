# AnyChat Android Package - Documentation Index

Welcome to the AnyChat Android SDK package! This index will help you find the right documentation.

## For End Users (App Developers)

If you're building an Android app and want to use AnyChat SDK:

1. **Start here**: [QUICKSTART.md](QUICKSTART.md) - Get up and running in 5 minutes
2. **Full guide**: [README.md](README.md) - Complete documentation with examples
3. **Examples**: [example/](example/) - Working Android app demonstrating all features

### Installation

```kotlin
dependencies {
    implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
}
```

## For Contributors & Maintainers

If you're contributing to the SDK or maintaining the package:

1. **Structure**: [STRUCTURE.md](STRUCTURE.md) - Complete package architecture overview
2. **Reference**: [REFERENCE.md](REFERENCE.md) - Quick command reference card
3. **Publishing**: [PUBLISHING.md](PUBLISHING.md) - Maven Central publishing guide
4. **Changes**: [CHANGELOG.md](CHANGELOG.md) - Version history

### Development Setup

```bash
git clone --recurse-submodules https://github.com/yzhgit/anychat-sdk.git
cd anychat-sdk/packages/android
./verify.sh
```

## Documentation Files

### User Documentation

| File | Purpose | Audience |
|------|---------|----------|
| [README.md](README.md) | Main package documentation | End users |
| [QUICKSTART.md](QUICKSTART.md) | 5-minute quick start | New users |
| [example/README.md](example/README.md) | Example app guide | Developers learning the SDK |

### Developer Documentation

| File | Purpose | Audience |
|------|---------|----------|
| [STRUCTURE.md](STRUCTURE.md) | Package architecture | Contributors, maintainers |
| [REFERENCE.md](REFERENCE.md) | Command quick reference | Developers, maintainers |
| [PUBLISHING.md](PUBLISHING.md) | Maven publishing guide | Maintainers, release managers |
| [CHANGELOG.md](CHANGELOG.md) | Version history | All audiences |

### Build Configuration

| File | Purpose |
|------|---------|
| [build.gradle.kts](build.gradle.kts) | Main Gradle build script (Kotlin DSL) |
| [settings.gradle.kts](settings.gradle.kts) | Gradle project settings |
| [gradle.properties](gradle.properties) | Build properties and version |
| [proguard-rules.pro](proguard-rules.pro) | ProGuard/R8 rules for release builds |

### Utility Scripts

| File | Purpose |
|------|---------|
| [verify.sh](verify.sh) | Verify package structure is complete |

## Common Tasks

### I want to...

**...use the SDK in my app**
→ See [QUICKSTART.md](QUICKSTART.md)

**...see working examples**
→ Check [example/](example/) directory

**...understand the architecture**
→ Read [STRUCTURE.md](STRUCTURE.md)

**...build the library locally**
→ See [REFERENCE.md](REFERENCE.md) "Build & Test" section

**...publish to Maven Central**
→ Follow [PUBLISHING.md](PUBLISHING.md)

**...report a bug or request a feature**
→ Open an issue: https://github.com/yzhgit/anychat-sdk/issues

**...contribute code**
→ See [STRUCTURE.md](STRUCTURE.md) and the main repository's CONTRIBUTING guide

## Quick Links

### External Resources

- **Backend API Docs**: https://yzhgit.github.io/anychat-server
- **Backend Repository**: https://github.com/yzhgit/anychat-server
- **SDK Repository**: https://github.com/yzhgit/anychat-sdk
- **Issue Tracker**: https://github.com/yzhgit/anychat-sdk/issues
- **Discussions**: https://github.com/yzhgit/anychat-sdk/discussions

### Maven Central

- **Group**: `io.github.yzhgit`
- **Artifact**: `anychat-sdk-android`
- **Latest Version**: 0.1.0
- **Search**: https://search.maven.org/artifact/io.github.yzhgit/anychat-sdk-android

## Package Structure

```
packages/android/
├── 📚 Documentation
│   ├── INDEX.md              ← You are here!
│   ├── README.md             Main documentation
│   ├── QUICKSTART.md         Quick start guide
│   ├── STRUCTURE.md          Architecture overview
│   ├── REFERENCE.md          Command reference
│   ├── PUBLISHING.md         Publishing guide
│   └── CHANGELOG.md          Version history
│
├── 🔧 Configuration
│   ├── build.gradle.kts      Main build script
│   ├── settings.gradle.kts   Project settings
│   ├── gradle.properties     Build properties
│   └── proguard-rules.pro    ProGuard rules
│
├── 📱 Library Source
│   └── src/main/
│       └── AndroidManifest.xml
│   (Actual sources are in ../../bindings/android/)
│
├── 📦 Example App
│   └── example/
│       ├── README.md
│       ├── build.gradle.kts
│       └── src/main/
│
└── 🛠️ Utilities
    └── verify.sh             Structure verification
```

## Support & Community

- **Questions**: Use [GitHub Discussions](https://github.com/yzhgit/anychat-sdk/discussions)
- **Bug Reports**: File an [issue](https://github.com/yzhgit/anychat-sdk/issues)
- **Security Issues**: See SECURITY.md in the main repository
- **Contributing**: See CONTRIBUTING.md in the main repository

## License

This package is part of the AnyChat SDK and is released under the MIT License.
See [../../LICENSE](../../LICENSE) for details.

---

**Need help?** Start with [QUICKSTART.md](QUICKSTART.md) for basic usage, or [STRUCTURE.md](STRUCTURE.md) for development.
