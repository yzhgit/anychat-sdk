/**
 * @anychat/sdk - AnyChat Web SDK
 *
 * This package wraps the AnyChat WebAssembly bindings and provides
 * a clean npm-publishable interface for web applications.
 */

// Re-export everything from the bindings
export * from '../../bindings/web/src/index';
export * from '../../bindings/web/src/types';
export * from '../../bindings/web/src/AnyChatClient';
export * from '../../bindings/web/src/EventEmitter';

// Re-export the main factory function as default
export { default } from '../../bindings/web/src/index';
