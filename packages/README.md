# AnyChat SDK Packages

This directory contains platform packages for AnyChat:

- `android/`
- `ios/`
- `flutter/`
- `web/`

## Directory Layout

```text
packages/
|-- android/
|-- ios/
|-- flutter/
`-- web/
```

## Common Prerequisites

From repository root:

```bash
git submodule update --init --recursive
cmake -B build -DBUILD_TESTS=ON
cmake --build build -j
```

## Platform Build Commands

### Android (`packages/android`)

> This package currently uses local `gradle` (no `gradlew` wrapper script in this folder).

```bash
cd packages/android
gradle clean assembleRelease
gradle test
```

### iOS/macOS (`packages/ios`)

```bash
cd packages/ios
pod lib lint AnyChatSDK.podspec.json --allow-warnings
```

### Flutter (`packages/flutter`)

```bash
cd packages/flutter
flutter pub get
dart run ffigen --config ffigen.yaml
flutter analyze
flutter test
```

### Web (`packages/web`)

Build WebAssembly bindings with Emscripten:

```bash
cd packages/web
emcmake cmake -B build -DBUILD_TESTS=OFF
cmake --build build -j
```

Build outputs:
- `packages/web/build/anychat.js`
- `packages/web/build/anychat.wasm`

## Related Docs

- [Android README](android/README.md)
- [iOS README](ios/README.md)
- [Flutter README](flutter/README.md)
- [Web README](web/README.md)

## License

MIT. See [../LICENSE](../LICENSE).
