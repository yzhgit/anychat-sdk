#!/bin/bash

# Build script for @anychat/sdk package

set -e

echo "========================================="
echo "Building @anychat/sdk"
echo "========================================="

# Clean previous builds
echo ""
echo "Step 1: Cleaning previous builds..."
rm -rf dist lib

# Build bindings first if needed
echo ""
echo "Step 2: Checking bindings build..."
if [ ! -f "../../bindings/web/lib/anychat.js" ] || [ ! -f "../../bindings/web/lib/anychat.wasm" ]; then
    echo "  → Building bindings (this may take a while)..."
    cd ../../bindings/web
    npm run build 2>/dev/null || true
    ./build.sh
    cd ../../packages/web
else
    echo "  ✓ Bindings already built"
fi

# Compile TypeScript
echo ""
echo "Step 3: Compiling TypeScript..."
npx tsc
echo "  ✓ TypeScript compiled"

# Bundle with Rollup (creates ESM, CJS, and copies WASM files)
echo ""
echo "Step 4: Bundling with Rollup..."
npx rollup -c
echo "  ✓ Bundled successfully"

# Copy CJS type definitions
echo ""
echo "Step 5: Creating CommonJS type definitions..."
cp dist/index.d.ts dist/index.d.cts
echo "  ✓ Type definitions created"

echo ""
echo "========================================="
echo "Build complete!"
echo "========================================="
echo ""
echo "Output files:"
echo "  • dist/index.js        - ESM bundle"
echo "  • dist/index.cjs       - CommonJS bundle"
echo "  • dist/index.d.ts      - TypeScript definitions (ESM)"
echo "  • dist/index.d.cts     - TypeScript definitions (CJS)"
echo "  • lib/anychat.js       - WASM loader"
echo "  • lib/anychat.wasm     - WebAssembly binary"
echo ""
echo "Package is ready for publishing!"
echo ""
