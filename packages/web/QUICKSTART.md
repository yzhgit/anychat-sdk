# Quick Start Guide - @anychat/sdk

Get started with the AnyChat Web SDK in minutes.

## Installation

```bash
npm install @anychat/sdk
```

## Setup

### 1. Copy WASM Files

The SDK requires WASM files to be served from your public directory. Copy them from the package:

```bash
# For most projects
mkdir -p public/lib
cp node_modules/@anychat/sdk/lib/* public/lib/

# For Next.js
mkdir -p public/lib
cp node_modules/@anychat/sdk/lib/* public/lib/

# For Vite
mkdir -p public/lib
cp node_modules/@anychat/sdk/lib/* public/lib/
```

Or use a bundler plugin (see README.md for examples).

### 2. Initialize the Client

```typescript
import { createAnyChatClient, ConnectionState } from '@anychat/sdk';

// Initialize client
const client = await createAnyChatClient({
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1',
  deviceId: 'web-' + Math.random().toString(36).substring(2, 15),
  dbPath: ':memory:', // In-memory database for web
}, '/lib/anychat.js'); // Path to WASM loader

// Connect to server
client.connect();
```

### 3. Listen for Events

```typescript
// Connection state
client.on('connectionStateChanged', (state) => {
  if (state === ConnectionState.Connected) {
    console.log('Connected!');
  }
});

// Incoming messages
client.on('messageReceived', (message) => {
  console.log('New message:', message.content);
});
```

### 4. Login

```typescript
try {
  const token = await client.login('user@example.com', 'password');
  console.log('Logged in:', token);
} catch (error) {
  console.error('Login failed:', error);
}
```

### 5. Send Messages

```typescript
// Get conversations
const conversations = await client.getConversationList();

// Send a message
await client.sendTextMessage(conversations[0].convId, 'Hello!');
```

## Complete Example

```typescript
import { createAnyChatClient, ConnectionState } from '@anychat/sdk';

async function main() {
  // 1. Initialize
  const client = await createAnyChatClient({
    gatewayUrl: 'wss://api.anychat.io',
    apiBaseUrl: 'https://api.anychat.io/api/v1',
    deviceId: 'web-' + Date.now(),
    dbPath: ':memory:',
  }, '/lib/anychat.js');

  // 2. Setup event listeners
  client.on('connectionStateChanged', (state) => {
    console.log('State:', ConnectionState[state]);
  });

  client.on('messageReceived', (message) => {
    console.log('Message:', message.content);
  });

  // 3. Connect and login
  client.connect();

  const token = await client.login('user@example.com', 'password');
  console.log('Token:', token);

  // 4. Get conversations
  const conversations = await client.getConversationList();
  console.log('Conversations:', conversations.length);

  // 5. Send a message
  if (conversations.length > 0) {
    await client.sendTextMessage(
      conversations[0].convId,
      'Hello from AnyChat SDK!'
    );
  }
}

main().catch(console.error);
```

## Common Use Cases

### Auto-reconnect

```typescript
const client = await createAnyChatClient({
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1',
  deviceId: 'web-123',
  autoReconnect: true,           // Enable auto-reconnect
  maxReconnectAttempts: 5,       // Max retry attempts
  connectTimeoutMs: 30000,       // Connection timeout
}, '/lib/anychat.js');
```

### Handle Token Expiration

```typescript
client.on('authExpired', async () => {
  // Token expired, refresh it
  const oldToken = JSON.parse(localStorage.getItem('token'));
  const newToken = await client.refreshToken(oldToken.refreshToken);
  localStorage.setItem('token', JSON.stringify(newToken));
});
```

### Mark Messages as Read

```typescript
client.on('messageReceived', async (message) => {
  // Display message
  displayMessage(message);

  // Mark as read
  await client.markMessageRead(message.convId, message.messageId);
});
```

### Load Message History

```typescript
async function loadHistory(convId: string) {
  // Load last 50 messages
  const messages = await client.getMessageHistory(convId, 0, 50);

  // Load older messages (pagination)
  const oldestTimestamp = messages[0].timestamp;
  const olderMessages = await client.getMessageHistory(
    convId,
    oldestTimestamp,
    50
  );

  return [...olderMessages, ...messages];
}
```

### Friend Management

```typescript
// Get friend list
const friends = await client.getFriendList();

// Send friend request
await client.sendFriendRequest('user-id-123', 'Hello!');

// Handle friend request
client.on('friendRequest', async (request) => {
  // Accept or reject
  await client.handleFriendRequest(request.requestId, true); // Accept
});

// Get pending requests
const requests = await client.getPendingFriendRequests();
```

### Group Operations

```typescript
// Create a group
await client.createGroup('My Group', ['user1', 'user2']);

// Get group list
const groups = await client.getGroupList();

// Invite to group
await client.inviteToGroup(groups[0].groupId, ['user3']);

// Get group members
const members = await client.getGroupMembers(groups[0].groupId, 1, 20);
```

## TypeScript Support

The SDK includes full TypeScript definitions:

```typescript
import type {
  ClientConfig,
  Message,
  Conversation,
  ConnectionState,
  MessageType,
} from '@anychat/sdk';

// All types are available
const config: ClientConfig = {
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1',
  deviceId: 'web-123',
};
```

## Next Steps

- See [README.md](./README.md) for full API reference
- Check [example/](./example/) for a complete chat application
- Read [DEVELOPMENT.md](./DEVELOPMENT.md) for development guide

## Need Help?

- Issues: https://github.com/yzhgit/anychat-server/issues
- Documentation: https://yzhgit.github.io/anychat-server
