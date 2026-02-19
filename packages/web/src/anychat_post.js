// Emscripten post-js: runs after the WASM module is initialized.
// This file is automatically concatenated to the generated JavaScript output.

// Re-export the module for ES6 module compatibility
if (typeof module !== 'undefined' && module.exports) {
  module.exports = Module;
}

// Note: The TypeScript layer (index.ts) handles the high-level Promise-based API.
// This file just ensures the module is properly exported.
