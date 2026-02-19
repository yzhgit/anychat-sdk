// Emscripten post-js: runs after the WASM module is initialized.
// Wraps the raw Emscripten module into a clean JS/Promise API.

Module['AnyChatClient'] = class AnyChatClient {
  constructor(config) {
    this._config = config;
    // TODO: instantiate via Emscripten-generated binding
    this._handle = null;
  }

  async connect() {
    // TODO: call Module._anychat_connect(this._handle)
    throw new Error('Not yet implemented');
  }

  disconnect() {
    // TODO: call Module._anychat_disconnect(this._handle)
  }
};
