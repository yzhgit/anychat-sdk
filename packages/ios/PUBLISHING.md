# Publishing AnyChatSDK to CocoaPods

This guide explains how to publish AnyChatSDK to CocoaPods.

## Prerequisites

1. CocoaPods account
2. Git repository with tags
3. Valid podspec file
4. Built C library

## Package Structure

```
packages/ios/
├── AnyChatSDK.podspec          # CocoaPods specification
├── AnyChatSDK.podspec.json     # JSON version for validation
├── README.md                   # Package documentation
├── CHANGELOG.md                # Version history
├── verify_package.sh           # Verification script
└── Example/                    # Example iOS app
    ├── Podfile                 # CocoaPods dependencies
    ├── README.md               # Example app documentation
    ├── AnyChatSDKExample.xcodeproj/
    └── AnyChatSDKExample/
        ├── ContentView.swift   # SwiftUI app implementation
        ├── Info.plist          # App configuration
        └── Assets.xcassets/    # App assets
```

## Pre-publish Checklist

### 1. Verify Package Structure

Run the verification script:

```bash
cd packages/ios
./verify_package.sh
```

### 2. Build the Core Library

The podspec includes a `prepare_command` that builds the C library automatically, but you should test it locally first:

```bash
cd ../../  # Go to repo root
git submodule update --init --recursive

mkdir -p build
cd build

cmake ../core \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF

cmake --build . --target anychat_core --config Release
```

### 3. Test the Example App

```bash
cd packages/ios/Example
pod install
open AnyChatSDKExample.xcworkspace
```

Build and run the app in Xcode to verify everything works.

### 4. Validate the Podspec

Local validation (skips building):

```bash
pod spec lint AnyChatSDK.podspec --allow-warnings --skip-import-validation
```

Full validation (builds library - requires all dependencies):

```bash
pod spec lint AnyChatSDK.podspec --allow-warnings
```

### 5. Test Local Installation

Test installing the pod locally before publishing:

```bash
# In your test project's Podfile:
pod 'AnyChatSDK', :path => '/path/to/anychat-sdk/packages/ios'
```

## Publishing Steps

### 1. Register with CocoaPods Trunk

If you haven't already:

```bash
pod trunk register your-email@example.com 'Your Name' --description='MacBook Pro'
```

Verify the email sent to you.

### 2. Create a Git Tag

```bash
cd ../../  # Go to repo root
git tag v0.1.0
git push origin v0.1.0
```

The tag must match the version in the podspec.

### 3. Push to CocoaPods

```bash
cd packages/ios
pod trunk push AnyChatSDK.podspec
```

If you encounter issues, use verbose mode:

```bash
pod trunk push AnyChatSDK.podspec --verbose
```

### 4. Verify Publication

After a few minutes, verify the pod is available:

```bash
pod search AnyChatSDK
```

Or check on [CocoaPods.org](https://cocoapods.org/pods/AnyChatSDK)

## Post-publish

### Update Documentation

1. Update the main README.md with installation instructions
2. Add the CocoaPods badge:
   ```markdown
   [![CocoaPods](https://img.shields.io/cocoapods/v/AnyChatSDK.svg)](https://cocoapods.org/pods/AnyChatSDK)
   ```

### Announce the Release

- Create a GitHub release
- Update documentation site
- Notify users on social media

## Updating the Pod

For subsequent versions:

1. Update version in `AnyChatSDK.podspec` and `AnyChatSDK.podspec.json`
2. Update `CHANGELOG.md` with changes
3. Commit changes
4. Create and push new git tag
5. Push to CocoaPods trunk

```bash
# Example for version 0.2.0
git tag v0.2.0
git push origin v0.2.0
pod trunk push AnyChatSDK.podspec
```

## Troubleshooting

### Build Failures

If the `prepare_command` fails during pod installation:

1. Check CMake is available: `which cmake`
2. Check Xcode Command Line Tools: `xcode-select --install`
3. Verify submodules are initialized
4. Check build logs in `~/Library/Logs/CocoaPods/`

### Import Errors

If users can't import the module:

1. Verify `module.modulemap` is correct
2. Check `public_header_files` includes all necessary headers
3. Ensure header search paths are set correctly

### Linker Errors

If users get linker errors:

1. Verify `vendored_libraries` path is correct
2. Check all required system frameworks are listed
3. Ensure C++ standard library is linked (`libraries = 'c++'`)

### Version Conflicts

If there are dependency version conflicts:

1. Update deployment target if needed
2. Check Swift version compatibility
3. Verify platform requirements

## Additional Resources

- [CocoaPods Guides](https://guides.cocoapods.org/)
- [Podspec Syntax Reference](https://guides.cocoapods.org/syntax/podspec.html)
- [CocoaPods Trunk](https://guides.cocoapods.org/making/getting-setup-with-trunk.html)
- [AnyChatSDK Documentation](https://yzhgit.github.io/anychat-server)

## Support

For issues with the package:

- GitHub Issues: https://github.com/yzhgit/anychat-sdk/issues
- Email: sdk@anychat.io
