# Publishing to pub.dev

This guide explains how to publish the `anychat_sdk` package to pub.dev.

## Prerequisites

1. Ensure you have a verified account on [pub.dev](https://pub.dev)
2. Install the Dart SDK (comes with Flutter)
3. Verify your package is ready for publishing

## Pre-Publishing Checklist

### 1. Update Version

Update the version in `pubspec.yaml`:

```yaml
version: 0.1.0  # or your target version
```

### 2. Update CHANGELOG.md

Document all changes in `CHANGELOG.md`:

```markdown
## [0.1.0] - 2026-02-19

### Added
- Initial release
- Authentication module
- Messaging module
...
```

### 3. Validate Package

Run the pub.dev validation:

```bash
cd packages/flutter
flutter pub publish --dry-run
```

This will check for:
- Valid `pubspec.yaml`
- Required files (README.md, CHANGELOG.md, LICENSE)
- File size limits
- Proper package structure
- Valid dependencies

### 4. Check Package Score

Ensure your package will score well on pub.dev by checking:

- [x] README.md with clear documentation
- [x] LICENSE file (MIT)
- [x] Example app in `example/`
- [x] API documentation (dartdoc comments)
- [x] Changelog
- [x] Proper version constraints
- [x] No deprecated dependencies
- [x] Analysis passes with no errors

### 5. Generate Documentation

Ensure all public APIs have dartdoc comments:

```bash
dart doc .
```

## Publishing Steps

### 1. Login to pub.dev

```bash
dart pub login
```

Follow the authentication flow in your browser.

### 2. Dry Run

Always do a dry run first:

```bash
flutter pub publish --dry-run
```

Review the output carefully. Fix any warnings or errors.

### 3. Publish

When ready, publish the package:

```bash
flutter pub publish
```

Confirm when prompted:

```
Publishing anychat_sdk 0.1.0 to https://pub.dev:
|-- ...
'-- ...

Do you want to publish anychat_sdk 0.1.0 (y/N)? y
```

### 4. Verify Publication

After publishing:

1. Visit https://pub.dev/packages/anychat_sdk
2. Check the package page loads correctly
3. Verify the README renders properly
4. Check the API documentation
5. Test installation in a fresh project:

```bash
flutter create test_app
cd test_app
flutter pub add anychat_sdk
```

## Post-Publishing

### Tag the Release

Create a git tag for the release:

```bash
git tag -a v0.1.0 -m "Release version 0.1.0"
git push origin v0.1.0
```

### Create GitHub Release

1. Go to https://github.com/yzhgit/anychat-sdk/releases
2. Click "Draft a new release"
3. Select the tag `v0.1.0`
4. Copy the changelog content
5. Publish the release

### Monitor

- Check pub.dev package page for any issues
- Monitor the package score
- Respond to issues and questions on GitHub

## Common Issues

### Package Already Exists

If publishing fails because a version already exists:

```
Version 0.1.0 of anychat_sdk already exists.
```

You need to bump the version number in `pubspec.yaml`.

### File Size Limit

If the package is too large:

```
Package archive exceeds 100 MB
```

Check `.gitignore` to ensure build artifacts aren't included. Use `.pubignore` to exclude additional files.

### Invalid Dependency

If a dependency constraint is invalid:

```
Because anychat_sdk depends on package_name any which doesn't match any versions, version solving failed.
```

Update the dependency version constraint in `pubspec.yaml`.

### Missing Files

If required files are missing:

```
Missing required file: README.md
```

Ensure all required files exist:
- `README.md`
- `CHANGELOG.md`
- `LICENSE`
- `pubspec.yaml`

## Best Practices

1. **Semantic Versioning**: Follow [semver](https://semver.org/)
   - MAJOR version for incompatible API changes
   - MINOR version for backwards-compatible functionality
   - PATCH version for backwards-compatible bug fixes

2. **Changelog**: Keep detailed changelog entries

3. **Testing**: Test thoroughly before publishing

4. **Documentation**: Keep documentation up-to-date

5. **Examples**: Provide clear, working examples

6. **Deprecation**: Deprecate before removing APIs:
   ```dart
   @Deprecated('Use newMethod() instead')
   void oldMethod() { }
   ```

## Versioning Strategy

For this package:

- `0.1.x` - Initial development, unstable API
- `0.2.x` - API stabilization
- `1.0.0` - First stable release
- `1.x.x` - Stable, following semver

## Resources

- [Publishing packages](https://dart.dev/tools/pub/publishing)
- [Package layout conventions](https://dart.dev/tools/pub/package-layout)
- [Pubspec format](https://dart.dev/tools/pub/pubspec)
- [Writing package pages](https://dart.dev/guides/libraries/writing-package-pages)
