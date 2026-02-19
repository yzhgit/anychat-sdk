# AnyChat Web SDK

WebAssembly bindings for the AnyChat IM SDK, providing a complete instant messaging solution for web applications.

## Features

- **WebAssembly-based**: High performance C++ core compiled to WASM
- **Promise-based API**: Modern async/await syntax
- **Event-driven**: EventEmitter pattern for real-time updates
- **TypeScript support**: Full type definitions included
- **Lightweight**: Minimal bundle size with tree-shaking support
- **Cross-browser**: Works in all modern browsers

## Installation

### NPM

```bash
npm install @anychat/sdk-web
```

### Build from Source

```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build the SDK
cd /path/to/anychat-sdk
mkdir build-web
cd build-web
emcmake cmake .. -DBUILD_WEB_BINDING=ON -DBUILD_TESTS=OFF
emmake make -j$(nproc)

# The output files will be in bindings/web/:
# - anychat.js (JavaScript loader)
# - anychat.wasm (WebAssembly binary)
```

## Quick Start

### Basic Usage

```typescript
import { createAnyChatClient, ConnectionState } from '@anychat/sdk-web';

// Initialize the client
const client = await createAnyChatClient({
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1',
  deviceId: 'web-device-12345',
});

// Listen for connection state changes
client.on('connectionStateChanged', (state) => {
  console.log('Connection state:', state);
});

// Connect to the server
client.connect();

// Login
try {
  const token = await client.login('user@example.com', 'password');
  console.log('Logged in:', token);
} catch (error) {
  console.error('Login failed:', error);
}
```

### Sending Messages

```typescript
// Send a text message
await client.sendTextMessage('conversation-id', 'Hello, World!');

// Listen for incoming messages
client.on('messageReceived', (message) => {
  console.log('New message:', message);
});

// Get message history
const messages = await client.getMessageHistory('conversation-id', 0, 20);
```

### Managing Conversations

```typescript
// Get conversation list
const conversations = await client.getConversationList();

// Pin a conversation
await client.setConversationPinned('conv-id', true);

// Mute a conversation
await client.setConversationMuted('conv-id', true);

// Mark as read
await client.markConversationRead('conv-id');

// Listen for conversation updates
client.on('conversationUpdated', (conversation) => {
  console.log('Conversation updated:', conversation);
});
```

### Friend Management

```typescript
// Get friend list
const friends = await client.getFriendList();

// Send friend request
await client.sendFriendRequest('user-id', 'Hello!');

// Handle friend request
client.on('friendRequest', async (request) => {
  console.log('Friend request from:', request.fromUserInfo.username);
  // Accept the request
  await client.handleFriendRequest(request.requestId, true);
});

// Get pending friend requests
const requests = await client.getPendingFriendRequests();
```

### Group Chat

```typescript
// Get group list
const groups = await client.getGroupList();

// Create a group
await client.createGroup('My Group', ['user-id-1', 'user-id-2']);

// Join a group
await client.joinGroup('group-id', 'Hello everyone!');

// Invite users to group
await client.inviteToGroup('group-id', ['user-id-3']);

// Get group members
const members = await client.getGroupMembers('group-id', 1, 20);

// Listen for group invitations
client.on('groupInvited', ({ group, inviterId }) => {
  console.log(`Invited to group ${group.name} by ${inviterId}`);
});
```

## API Reference

### Client Configuration

```typescript
interface ClientConfig {
  gatewayUrl: string;          // WebSocket gateway URL
  apiBaseUrl: string;          // HTTP API base URL
  deviceId: string;            // Unique device identifier
  dbPath?: string;             // SQLite database path (default: ':memory:')
  connectTimeoutMs?: number;   // Connection timeout (default: 10000)
  maxReconnectAttempts?: number; // Max reconnection attempts (default: 5)
  autoReconnect?: boolean;     // Auto-reconnect on disconnect (default: true)
}
```

### Events

The client extends `EventEmitter` and emits the following events:

| Event | Payload | Description |
|-------|---------|-------------|
| `connectionStateChanged` | `ConnectionState` | Connection state changed |
| `messageReceived` | `Message` | New message received |
| `conversationUpdated` | `Conversation` | Conversation updated |
| `friendRequest` | `FriendRequest` | Friend request received |
| `friendListChanged` | `void` | Friend list changed |
| `groupInvited` | `{ group: Group, inviterId: string }` | Invited to a group |
| `groupUpdated` | `Group` | Group information updated |
| `authExpired` | `void` | Authentication token expired |

### Connection States

```typescript
enum ConnectionState {
  Disconnected = 0,
  Connecting = 1,
  Connected = 2,
  Reconnecting = 3,
}
```

### Message Types

```typescript
enum MessageType {
  Text = 0,
  Image = 1,
  File = 2,
  Audio = 3,
  Video = 4,
}
```

## Advanced Usage

### Custom WASM Module Path

By default, the SDK looks for the WASM module at `/lib/anychat.js`. You can specify a custom path:

```typescript
const client = await createAnyChatClient(config, '/custom/path/anychat.js');
```

### Manual Initialization

For more control over the initialization process:

```typescript
import { AnyChatClient } from '@anychat/sdk-web';

const client = new AnyChatClient(config);

// Load the WASM module manually
const wasmModule = await import('/lib/anychat.js');
const module = await wasmModule.default();

await client.initialize(module);
```

### Error Handling

All async methods throw `AnyChatError` on failure:

```typescript
import { AnyChatError } from '@anychat/sdk-web';

try {
  await client.sendTextMessage('conv-id', 'Hello');
} catch (error) {
  if (error instanceof AnyChatError) {
    console.error('AnyChat error:', error.message);
  }
}
```

### TypeScript Support

The SDK is written in TypeScript and includes full type definitions:

```typescript
import type {
  Message,
  Conversation,
  Friend,
  Group,
  AuthToken,
} from '@anychat/sdk-web';
```

## Browser Compatibility

- Chrome 57+
- Firefox 52+
- Safari 11+
- Edge 16+

Requires WebAssembly support. See [caniuse.com/wasm](https://caniuse.com/wasm).

## Example Application

See the `example/` directory for a complete chat application demonstrating SDK usage.

To run the example:

```bash
cd example
# Serve with any static file server
python3 -m http.server 8000
# Open http://localhost:8000 in your browser
```

## Performance Tips

1. **Use in-memory database for web**: Set `dbPath: ':memory:'` in the config
2. **Lazy load the WASM module**: Import only when needed
3. **Debounce frequent operations**: Like marking messages as read
4. **Implement pagination**: For large conversation/message lists

## Troubleshooting

### WASM module fails to load

Ensure your web server serves `.wasm` files with the correct MIME type:

```nginx
# Nginx
types {
    application/wasm wasm;
}

# Apache (.htaccess)
AddType application/wasm .wasm
```

### CORS errors

The WASM module must be served from the same origin or with proper CORS headers.

### Memory issues

The SDK uses a SQLite database. For web, use in-memory mode to avoid quota issues:

```typescript
const client = await createAnyChatClient({
  ...config,
  dbPath: ':memory:', // Use in-memory database
});
```

## License

MIT

## Related Links

- [Backend API Documentation](https://yzhgit.github.io/anychat-server)
- [Backend Repository](https://github.com/yzhgit/anychat-server)
- [Emscripten Documentation](https://emscripten.org/)
