/**
 * index.ts - Main entry point for AnyChat Web SDK
 */

import { AnyChatClient } from './AnyChatClient';
import type { ClientConfig } from './types';

export * from './types';
export * from './AnyChatClient';
export { EventEmitter } from './EventEmitter';

/**
 * Create and initialize an AnyChat client.
 *
 * @param config - Client configuration
 * @param wasmModulePath - Path to the compiled WASM module (anychat.js)
 * @returns Promise that resolves to an initialized AnyChatClient
 *
 * @example
 * ```typescript
 * import { createAnyChatClient, ConnectionState } from '@anychat/sdk-web';
 *
 * const client = await createAnyChatClient({
 *   gatewayUrl: 'wss://api.anychat.io',
 *   apiBaseUrl: 'https://api.anychat.io/api/v1',
 *   deviceId: 'web-browser-12345',
 * }, '/lib/anychat.js');
 *
 * // Listen for connection state changes
 * client.on('connectionStateChanged', (state) => {
 *   console.log('Connection state:', state);
 * });
 *
 * // Listen for incoming messages
 * client.on('messageReceived', (message) => {
 *   console.log('New message:', message);
 * });
 *
 * // Connect to the server
 * client.connect();
 *
 * // Login
 * try {
 *   const token = await client.login('user@example.com', 'password');
 *   console.log('Logged in successfully:', token);
 * } catch (error) {
 *   console.error('Login failed:', error);
 * }
 *
 * // Send a message
 * await client.sendTextMessage('conv-123', 'Hello, World!');
 *
 * // Get conversation list
 * const conversations = await client.getConversationList();
 * console.log('Conversations:', conversations);
 * ```
 */
export async function createAnyChatClient(
  config: ClientConfig,
  wasmModulePath: string = '/lib/anychat.js'
): Promise<AnyChatClient> {
  // Dynamically import the WASM module
  const createModule = await import(/* @vite-ignore */ wasmModulePath);

  // Create the WASM module instance
  const wasmModule = await createModule.default();

  // Create and initialize the client
  const client = new AnyChatClient(config);
  await client.initialize(wasmModule);

  return client;
}

/**
 * Default export for convenience
 */
export default {
  createAnyChatClient,
  AnyChatClient,
};
