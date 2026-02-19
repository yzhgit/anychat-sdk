# Developer Reference Card

Quick reference for common tasks with the AnyChat Android package.

## Quick Commands

### Build & Test

```bash
# Build library (release)
./gradlew assembleRelease

# Build library (debug)
./gradlew assembleDebug

# Run unit tests
./gradlew test

# Run instrumented tests
./gradlew connectedAndroidTest

# Build example app
./gradlew :example:assembleDebug

# Install example on device
./gradlew :example:installDebug

# Clean build
./gradlew clean
```

### Publishing

```bash
# Publish to local Maven repository
./gradlew publishToMavenLocal

# Publish to Maven Central staging
./gradlew publish

# Verify package structure
./verify.sh
```

### Gradle Tasks

```bash
# List all tasks
./gradlew tasks

# Show dependencies
./gradlew dependencies

# Show project info
./gradlew properties
```

## File Locations

### Build Outputs

```
build/outputs/
├── aar/                          # Android Archive files
│   ├── android-debug.aar
│   └── android-release.aar
├── logs/                         # Build logs
└── native-debug-symbols/         # Native debugging symbols
```

### Maven Publication

```
build/publications/release/
├── pom-default.xml               # Maven POM
└── module.json                   # Gradle metadata
```

## Gradle Properties

### In `gradle.properties`

```properties
VERSION_NAME=0.1.0                # Package version
GROUP=io.github.yzhgit           # Maven groupId
```

### In `~/.gradle/gradle.properties` (credentials)

```properties
OSSRH_USERNAME=your_username
OSSRH_PASSWORD=your_password
signing.keyId=GPG_KEY_ID
signing.password=GPG_PASSWORD
signing.secretKeyRingFile=/path/to/secring.gpg
```

## Environment Variables

```bash
# Alternative to gradle.properties
export OSSRH_USERNAME=your_username
export OSSRH_PASSWORD=your_password
```

## Android Studio Tasks

### Build Variants

- **Debug**: Development build with debugging info
- **Release**: Production build with optimizations

Switch in: `Build > Select Build Variant`

### Run Configurations

1. **Library** - Build the AAR
2. **Example** - Run the example app

## Source Locations

| Component | Location |
|-----------|----------|
| Kotlin sources | `../../packages/android/src/main/kotlin/` |
| JNI C++ sources | `../../packages/android/src/main/cpp/` |
| Core C++ SDK | `../../core/` |
| CMake config | `../../CMakeLists.txt` |
| Third-party libs | `../../thirdparty/` |

## Common Issues & Solutions

### Issue: Native library not found

**Solution**: Ensure CMake and NDK are installed
```bash
# In Android Studio
Tools > SDK Manager > SDK Tools
☑ CMake
☑ NDK (Side by side)
```

### Issue: Gradle sync failed

**Solution**: Invalidate caches
```
File > Invalidate Caches > Invalidate and Restart
```

### Issue: Signing failed during publish

**Solution**: Check GPG configuration
```bash
# Verify GPG key exists
gpg --list-secret-keys

# Re-export if needed
gpg --export-secret-keys KEY_ID > ~/.gnupg/secring.gpg
```

### Issue: Submodules not initialized

**Solution**: Initialize submodules
```bash
git submodule update --init --recursive
```

## Version Bumping

### Patch Release (0.1.0 → 0.1.1)

```bash
# Edit gradle.properties
VERSION_NAME=0.1.1

# Update changelog
nano CHANGELOG.md

# Commit and tag
git commit -am "Bump version to 0.1.1"
git tag -a v0.1.1 -m "Release v0.1.1"
git push origin main --tags
```

### Minor Release (0.1.0 → 0.2.0)

Same as patch, update major version.

### Major Release (0.1.0 → 1.0.0)

Same as above, consider:
- Update documentation
- Migration guide for breaking changes
- Announcement on GitHub Releases

## Testing Locally

### Test in another project

```bash
# 1. Publish to Maven Local
./gradlew publishToMavenLocal

# 2. In test project's build.gradle.kts
repositories {
    mavenLocal()
}
dependencies {
    implementation("io.github.yzhgit:anychat-sdk-android:0.1.0")
}
```

## Documentation Updates

After changes, update:

- [ ] `README.md` - User-facing documentation
- [ ] `CHANGELOG.md` - Version history
- [ ] `example/MainActivity.kt` - Usage examples
- [ ] API docs (if public API changes)

## Pre-publish Checklist

- [ ] All tests pass
- [ ] Example app runs successfully
- [ ] Version bumped in `gradle.properties`
- [ ] `CHANGELOG.md` updated
- [ ] Git committed and tagged
- [ ] Tested with `publishToMavenLocal`
- [ ] Signing configured (if first time)
- [ ] POM metadata correct

## Publishing Workflow

1. **Local build**: `./gradlew assembleRelease`
2. **Test locally**: `./gradlew publishToMavenLocal`
3. **Verify**: Test in example app
4. **Publish**: `./gradlew publish`
5. **Release on Sonatype**: Login → Close → Release
6. **Verify**: Check Maven Central after 10-30 min
7. **Announce**: GitHub Release, update docs

## Useful Links

- [Gradle Tasks](https://docs.gradle.org/current/userguide/command_line_interface.html)
- [Android Library Publishing](https://developer.android.com/studio/build/maven-publish-plugin)
- [Maven Central Guide](https://central.sonatype.org/publish/)
- [Semantic Versioning](https://semver.org/)

## Get Help

```bash
# Gradle help
./gradlew help

# Task help
./gradlew help --task publish

# Verbose build
./gradlew assembleRelease --info

# Debug build
./gradlew assembleRelease --debug
```

---

**Pro Tip**: Bookmark this file and the `verify.sh` script for quick reference!
