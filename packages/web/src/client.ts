import type { ClientConfig, Message } from './types';

export class AnyChatClient {
  private config: ClientConfig;
  private _wasmModule: unknown = null;

  constructor(config: ClientConfig) {
    this.config = config;
  }

  async connect(): Promise<void> {
    // TODO: load WASM module and call anychat_connect
    throw new Error('connect() not yet implemented');
  }

  disconnect(): void {
    // TODO
  }

  onMessage(handler: (msg: Message) => void): () => void {
    // TODO: subscribe to WebSocket messages
    return () => {}; // unsubscribe
  }
}
