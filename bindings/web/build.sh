#!/bin/bash
#
# build.sh - Build script for AnyChat Web SDK
#
# This script builds the Web bindings using Emscripten.
# Prerequisites:
#   - Emscripten SDK installed and activated
#   - Git submodules initialized
#
# Usage:
#   ./build.sh [debug|release]

set -e

# Determine build type
BUILD_TYPE="${1:-Release}"
if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
    echo "Invalid build type: $BUILD_TYPE"
    echo "Usage: $0 [Debug|Release]"
    exit 1
fi

# Check if Emscripten is available
if ! command -v emcmake &> /dev/null; then
    echo "Error: Emscripten not found. Please install and activate the Emscripten SDK."
    echo "See: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WEB_BINDING_DIR="$PROJECT_ROOT/bindings/web"
BUILD_DIR="$PROJECT_ROOT/build-web-$BUILD_TYPE"

echo "========================================"
echo "Building AnyChat Web SDK"
echo "========================================"
echo "Build type: $BUILD_TYPE"
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring CMake..."
emcmake cmake "$PROJECT_ROOT" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_WEB_BINDING=ON \
    -DBUILD_TESTS=OFF \
    -DBUILD_ANDROID_BINDING=OFF \
    -DBUILD_IOS_BINDING=OFF

# Build
echo ""
echo "Building..."
emmake make -j$(nproc) anychat_wasm

# Create lib directory in web bindings
mkdir -p "$WEB_BINDING_DIR/lib"

# Copy output files
echo ""
echo "Copying output files..."
cp "$BUILD_DIR/bindings/web/anychat.js" "$WEB_BINDING_DIR/lib/"
cp "$BUILD_DIR/bindings/web/anychat.wasm" "$WEB_BINDING_DIR/lib/"

# Build TypeScript
echo ""
echo "Building TypeScript..."
cd "$WEB_BINDING_DIR"
if command -v npm &> /dev/null; then
    npm install
    npm run build
else
    echo "Warning: npm not found, skipping TypeScript build"
fi

echo ""
echo "========================================"
echo "Build complete!"
echo "========================================"
echo "Output files:"
echo "  - $WEB_BINDING_DIR/lib/anychat.js"
echo "  - $WEB_BINDING_DIR/lib/anychat.wasm"
echo "  - $WEB_BINDING_DIR/dist/ (TypeScript build)"
echo ""
echo "To test the example app:"
echo "  cd $WEB_BINDING_DIR/example"
echo "  python3 -m http.server 8000"
echo "  Open http://localhost:8000 in your browser"
echo ""
