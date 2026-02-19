# Development Guide

This guide explains how to develop and publish the `@anychat/sdk` package.

## Project Structure

```
packages/web/
├── src/                 # Source files (re-exports from bindings)
│   ├── index.ts        # Main entry point
│   ├── types.ts        # Type re-exports
│   └── client.ts       # Client re-exports
├── example/            # Example application
│   ├── src/
│   │   └── app.ts      # Example app code
│   ├── index.html      # HTML entry point
│   └── package.json    # Example dependencies
├── dist/               # Built files (generated)
│   ├── index.js        # ESM bundle
│   ├── index.cjs       # CommonJS bundle
│   ├── index.d.ts      # TypeScript definitions
│   └── index.d.cts     # CommonJS TypeScript definitions
├── lib/                # WASM files (copied from bindings)
│   ├── anychat.js      # WASM loader
│   └── anychat.wasm    # WebAssembly binary
├── package.json        # Package configuration
├── tsconfig.json       # TypeScript configuration
├── rollup.config.js    # Bundler configuration
├── build.sh            # Build script
├── README.md           # User documentation
├── CHANGELOG.md        # Version history
├── LICENSE             # MIT license
└── .npmignore          # npm publish exclusions
```

## Development Workflow

### 1. Initial Setup

Install dependencies:

```bash
npm install
```

Make sure the bindings are built:

```bash
cd ../../bindings/web
npm install
./build.sh
cd ../../packages/web
```

### 2. Development

The package simply re-exports from `../../bindings/web/src/`, so most development happens in the bindings directory.

To work on the package wrapper:

1. Edit files in `src/`
2. Run TypeScript compiler:
   ```bash
   npx tsc
   ```

### 3. Building

Build the complete package:

```bash
npm run build
```

This will:
1. Build the bindings (if needed)
2. Copy WASM files to `lib/`
3. Compile TypeScript to JavaScript
4. Bundle with Rollup (ESM + CommonJS)
5. Generate TypeScript definitions

Or use the build script directly:

```bash
./build.sh
```

### 4. Testing Locally

#### Link the Package

```bash
npm link
```

#### Use in Another Project

```bash
cd /path/to/your/project
npm link @anychat/sdk
```

#### Test with Example App

```bash
cd example
npm install
npm run dev
```

This starts a development server at http://localhost:5173

### 5. Publishing

#### Pre-publish Checklist

- [ ] Update version in `package.json`
- [ ] Update `CHANGELOG.md` with changes
- [ ] Run tests (when available)
- [ ] Build the package: `npm run build`
- [ ] Check package contents: `npm pack --dry-run`
- [ ] Verify files are correct

#### Publish to npm

```bash
# Login to npm (first time only)
npm login

# Publish (public package)
npm publish --access public

# Or for beta versions
npm publish --tag beta
```

#### Post-publish

- Tag the release in Git:
  ```bash
  git tag v0.1.0
  git push origin v0.1.0
  ```
- Update documentation
- Announce the release

## Package Configuration

### package.json

Key fields:

- `name`: `@anychat/sdk` (scoped package)
- `version`: Semantic versioning (e.g., `0.1.0`)
- `main`: CommonJS entry point (`dist/index.cjs`)
- `module`: ESM entry point (`dist/index.js`)
- `types`: TypeScript definitions (`dist/index.d.ts`)
- `exports`: Modern package exports (supports both ESM and CJS)
- `files`: Files to include in npm package

### tsconfig.json

- `target`: ES2020 for modern JavaScript
- `module`: ES2020 for ESM output
- `declaration`: Generate `.d.ts` files
- `strict`: Enable all strict type checks

### rollup.config.js

Bundles the package in multiple formats:

1. **ESM**: `dist/index.js` - Modern import/export syntax
2. **CommonJS**: `dist/index.cjs` - require() syntax for Node.js
3. **Type definitions**: `dist/index.d.ts` and `dist/index.d.cts`

Also copies WASM files from bindings to `lib/`.

## Version Management

Follow [Semantic Versioning](https://semver.org/):

- **Major** (1.0.0): Breaking changes
- **Minor** (0.1.0): New features (backwards compatible)
- **Patch** (0.0.1): Bug fixes

Update version:

```bash
npm version patch  # 0.1.0 -> 0.1.1
npm version minor  # 0.1.1 -> 0.2.0
npm version major  # 0.2.0 -> 1.0.0
```

This updates `package.json` and creates a git tag.

## Troubleshooting

### WASM files not found

Make sure the bindings are built and WASM files exist:

```bash
ls ../../bindings/web/lib/anychat.{js,wasm}
```

If missing, build the bindings:

```bash
cd ../../bindings/web && ./build.sh
```

### TypeScript errors

Check TypeScript paths are correct:

```bash
npx tsc --noEmit
```

### Module resolution issues

Make sure `moduleResolution` is set to `"node"` in `tsconfig.json`.

### npm publish fails

Common issues:

1. **Not logged in**: Run `npm login`
2. **Version already exists**: Update version in `package.json`
3. **Scope not allowed**: Use `--access public` for scoped packages
4. **Files missing**: Check `.npmignore` and `files` field in `package.json`

## Best Practices

1. **Always build before publishing**: Run `npm run build` to ensure clean build
2. **Test locally first**: Use `npm link` to test in another project
3. **Update changelog**: Document all changes in `CHANGELOG.md`
4. **Version appropriately**: Follow semantic versioning
5. **Review package contents**: Use `npm pack` to see what will be published
6. **Keep dependencies minimal**: This package has no runtime dependencies
7. **Document breaking changes**: Clearly mark breaking changes in changelog

## CI/CD Integration

Example GitHub Actions workflow for automated publishing:

```yaml
name: Publish Package

on:
  push:
    tags:
      - 'v*'

jobs:
  publish:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - uses: actions/setup-node@v3
        with:
          node-version: '18'
          registry-url: 'https://registry.npmjs.org'

      - name: Install dependencies
        run: npm ci

      - name: Build
        run: npm run build

      - name: Publish
        run: npm publish --access public
        env:
          NODE_AUTH_TOKEN: ${{secrets.NPM_TOKEN}}
```

## Resources

- [npm Documentation](https://docs.npmjs.com/)
- [TypeScript Handbook](https://www.typescriptlang.org/docs/)
- [Rollup Documentation](https://rollupjs.org/)
- [Semantic Versioning](https://semver.org/)
