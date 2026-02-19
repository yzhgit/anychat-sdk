# @anychat/sdk

Official JavaScript/TypeScript SDK for the AnyChat instant messaging system. Built with WebAssembly for high performance and native-like functionality in web browsers.

## Features

- Full-featured IM SDK with WebSocket real-time messaging
- Built with WebAssembly (WASM) for optimal performance
- TypeScript support with full type definitions
- Event-driven API with Promise-based methods
- Automatic reconnection and connection management
- Local database for message persistence (SQLite via WASM)
- Support for text, image, file, audio, and video messages
- Friend and group management
- Conversation management with read status, pinning, and muting

## Installation

```bash
npm install @anychat/sdk
```

## Quick Start

```typescript
import { createAnyChatClient, ConnectionState } from '@anychat/sdk';

// Initialize the client
const client = await createAnyChatClient({
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1',
  deviceId: 'web-browser-12345',
  dbPath: ':memory:', // Use in-memory database for web
}, '/lib/anychat.js'); // Path to WASM module

// Listen for connection state changes
client.on('connectionStateChanged', (state) => {
  console.log('Connection state:', state);
});

// Listen for incoming messages
client.on('messageReceived', (message) => {
  console.log('New message:', message);
});

// Connect to the server
client.connect();

// Login
try {
  const token = await client.login('user@example.com', 'password');
  console.log('Logged in successfully:', token);
} catch (error) {
  console.error('Login failed:', error);
}

// Send a message
await client.sendTextMessage('conv-123', 'Hello, World!');

// Get conversation list
const conversations = await client.getConversationList();
console.log('Conversations:', conversations);
```

## WASM Module Setup

The SDK requires the WASM module files to be served alongside your application. You need to copy the WASM files from `node_modules/@anychat/sdk/lib/` to your public directory:

### Vite

```javascript
// vite.config.js
import { defineConfig } from 'vite';

export default defineConfig({
  publicDir: 'public',
  // Copy WASM files to public directory
  build: {
    rollupOptions: {
      external: []
    }
  }
});
```

Then copy the files:
```bash
cp node_modules/@anychat/sdk/lib/* public/lib/
```

### Webpack

```javascript
// webpack.config.js
const CopyPlugin = require('copy-webpack-plugin');

module.exports = {
  plugins: [
    new CopyPlugin({
      patterns: [
        { from: 'node_modules/@anychat/sdk/lib', to: 'lib' }
      ],
    }),
  ],
};
```

### Next.js

Add to your `next.config.js`:

```javascript
module.exports = {
  webpack: (config) => {
    config.plugins.push(
      new (require('copy-webpack-plugin'))({
        patterns: [
          { from: 'node_modules/@anychat/sdk/lib', to: '../public/lib' }
        ],
      })
    );
    return config;
  },
};
```

## API Reference

### Client Initialization

```typescript
createAnyChatClient(config: ClientConfig, wasmModulePath?: string): Promise<AnyChatClient>
```

#### ClientConfig

```typescript
interface ClientConfig {
  gatewayUrl: string;          // WebSocket gateway URL
  apiBaseUrl: string;          // HTTP API base URL
  deviceId: string;            // Unique device identifier
  dbPath?: string;             // Database path (default: ':memory:')
  connectTimeoutMs?: number;   // Connection timeout (default: 30000)
  maxReconnectAttempts?: number; // Max reconnection attempts (default: 5)
  autoReconnect?: boolean;     // Enable auto-reconnect (default: true)
}
```

### Events

The client extends `EventEmitter` and emits the following events:

- `connectionStateChanged`: Connection state changed (ConnectionState)
- `messageReceived`: New message received (Message)
- `conversationUpdated`: Conversation updated (Conversation)
- `friendRequest`: Friend request received (FriendRequest)
- `friendListChanged`: Friend list changed
- `groupInvited`: Invited to a group ({ group: Group, inviterId: string })
- `groupUpdated`: Group information updated (Group)
- `authExpired`: Authentication token expired

### Authentication

```typescript
// Login
client.login(account: string, password: string, deviceType?: string): Promise<AuthToken>

// Register
client.register(
  phoneOrEmail: string,
  password: string,
  verifyCode: string,
  deviceType?: string,
  nickname?: string
): Promise<AuthToken>

// Logout
client.logout(): Promise<void>

// Refresh token
client.refreshToken(refreshToken: string): Promise<AuthToken>

// Check login status
client.isLoggedIn(): boolean
```

### Connection Management

```typescript
// Connect to server
client.connect(): void

// Disconnect from server
client.disconnect(): void

// Get connection state
client.getConnectionState(): ConnectionState
```

### Message Operations

```typescript
// Send text message
client.sendTextMessage(sessionId: string, content: string): Promise<void>

// Get message history
client.getMessageHistory(
  sessionId: string,
  beforeTimestamp?: number,
  limit?: number
): Promise<Message[]>

// Mark message as read
client.markMessageRead(sessionId: string, messageId: string): Promise<void>
```

### Conversation Operations

```typescript
// Get conversation list
client.getConversationList(): Promise<Conversation[]>

// Mark conversation as read
client.markConversationRead(convId: string): Promise<void>

// Set conversation pinned status
client.setConversationPinned(convId: string, pinned: boolean): Promise<void>

// Set conversation muted status
client.setConversationMuted(convId: string, muted: boolean): Promise<void>

// Delete conversation
client.deleteConversation(convId: string): Promise<void>
```

### Friend Operations

```typescript
// Get friend list
client.getFriendList(): Promise<Friend[]>

// Send friend request
client.sendFriendRequest(toUserId: string, message?: string): Promise<void>

// Handle friend request
client.handleFriendRequest(requestId: number, accept: boolean): Promise<void>

// Get pending friend requests
client.getPendingFriendRequests(): Promise<FriendRequest[]>

// Delete friend
client.deleteFriend(friendId: string): Promise<void>
```

### Group Operations

```typescript
// Get group list
client.getGroupList(): Promise<Group[]>

// Create group
client.createGroup(name: string, memberIds: string[]): Promise<void>

// Join group
client.joinGroup(groupId: string, message?: string): Promise<void>

// Invite to group
client.inviteToGroup(groupId: string, userIds: string[]): Promise<void>

// Quit group
client.quitGroup(groupId: string): Promise<void>

// Get group members
client.getGroupMembers(
  groupId: string,
  page?: number,
  pageSize?: number
): Promise<GroupMember[]>
```

## Type Definitions

All TypeScript type definitions are included. Key types:

- `ClientConfig` - Client configuration
- `AuthToken` - Authentication token
- `Message` - Chat message
- `Conversation` - Conversation/session
- `Friend` - Friend information
- `FriendRequest` - Friend request
- `Group` - Group information
- `GroupMember` - Group member
- `ConnectionState` - Connection state enum
- `MessageType` - Message type enum
- `ConversationType` - Conversation type enum

## Browser Support

- Chrome/Edge 90+
- Firefox 88+
- Safari 15+
- Opera 76+

WebAssembly support is required.

## Examples

See the [example](./example/) directory for a complete web application example.

## Related Projects

- [Backend API Documentation](https://yzhgit.github.io/anychat-server)
- [Backend Repository](https://github.com/yzhgit/anychat-server)

## License

MIT License - see [LICENSE](../../LICENSE) for details.

## Support

For issues and questions:
- GitHub Issues: https://github.com/yzhgit/anychat-server/issues
- Documentation: https://yzhgit.github.io/anychat-server
