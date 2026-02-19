#!/bin/bash
# Quick verification script to check package structure

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "🔍 Verifying AnyChat Android Package Structure..."
echo ""

# Check required files
echo "✅ Checking required files..."
required_files=(
    "build.gradle.kts"
    "settings.gradle.kts"
    "gradle.properties"
    "src/main/AndroidManifest.xml"
    "proguard-rules.pro"
    "README.md"
    "CHANGELOG.md"
    "PUBLISHING.md"
    ".gitignore"
)

for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (missing)"
        exit 1
    fi
done

echo ""
echo "✅ Checking bindings source directory..."
if [ -d "../../bindings/android/src/main/kotlin" ]; then
    kotlin_files=$(find ../../bindings/android/src/main/kotlin -name "*.kt" | wc -l)
    echo "  ✓ Found $kotlin_files Kotlin source files"
else
    echo "  ✗ Bindings directory not found"
    exit 1
fi

echo ""
echo "✅ Checking example app..."
example_files=(
    "example/build.gradle.kts"
    "example/src/main/AndroidManifest.xml"
    "example/src/main/java/com/anychat/example/MainActivity.kt"
    "example/README.md"
)

for file in "${example_files[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ✗ $file (missing)"
        exit 1
    fi
done

echo ""
echo "✅ Checking CMakeLists.txt..."
if [ -f "../../CMakeLists.txt" ]; then
    echo "  ✓ Root CMakeLists.txt found"
else
    echo "  ⚠️  Root CMakeLists.txt not found (required for native build)"
fi

echo ""
echo "✅ Checking package metadata..."
if grep -q "io.github.yzhgit" build.gradle.kts; then
    echo "  ✓ GroupId: io.github.yzhgit"
fi
if grep -q "anychat-sdk-android" build.gradle.kts; then
    echo "  ✓ ArtifactId: anychat-sdk-android"
fi

version=$(grep "VERSION_NAME=" gradle.properties | cut -d'=' -f2)
echo "  ✓ Version: $version"

echo ""
echo "📦 Package Structure Verification Complete!"
echo ""
echo "Next steps:"
echo "  1. Open Android Studio and import this directory"
echo "  2. Let Gradle sync complete"
echo "  3. Build the library: ./gradlew assembleRelease"
echo "  4. Run example app: ./gradlew :example:installDebug"
echo "  5. Publish locally for testing: ./gradlew publishToMavenLocal"
echo ""
echo "For publishing to Maven Central, see PUBLISHING.md"
