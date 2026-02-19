# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-02-19

### Added
- Initial release of @anychat/sdk
- WebAssembly-based client implementation
- Full TypeScript support with type definitions
- Event-driven API with Promise-based methods
- Connection management with automatic reconnection
- Authentication (login, register, logout, token refresh)
- Message operations (send, receive, history, read status)
- Conversation management (list, read status, pin, mute, delete)
- Friend operations (list, add, accept/reject requests, delete)
- Group operations (list, create, join, invite, quit, member list)
- Local database for message persistence (SQLite via WASM)
- ESM and CommonJS module support
- Comprehensive documentation and examples

### Features
- Real-time messaging via WebSocket
- Support for text, image, file, audio, and video messages
- Offline message synchronization
- Connection state monitoring
- Automatic token refresh
- Device management
- Conversation unread count tracking

## [Unreleased]

### Planned
- Message editing and deletion
- File upload with progress tracking
- Voice and video call support
- Push notification integration
- Message reactions and mentions
- Read receipts for group messages
- Typing indicators
- Message search
- Conversation archiving
- User presence status
