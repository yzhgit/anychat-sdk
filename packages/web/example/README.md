# AnyChat SDK Example

This is a simple example demonstrating how to use the `@anychat/sdk` package in a web application.

## Features Demonstrated

- Client initialization with WASM module loading
- User authentication (login)
- Connection state monitoring
- Real-time message receiving
- Conversation list display
- Message history loading
- Sending text messages
- Event-driven architecture

## Getting Started

### 1. Install Dependencies

```bash
npm install
```

### 2. Copy WASM Files

The WASM files need to be available in the public directory. After building the bindings:

```bash
mkdir -p public/lib
cp ../../bindings/web/lib/* public/lib/
```

### 3. Run Development Server

```bash
npm run dev
```

This will start a Vite development server at http://localhost:5173

### 4. Build for Production

```bash
npm run build
```

The built files will be in the `dist` directory.

## Usage

### Login

Use any valid credentials from your AnyChat server:
- Email or phone number
- Password

### Chat

Once logged in, you can:
- View your conversation list
- Click on a conversation to view messages
- Send messages by typing in the input field and clicking Send or pressing Enter

## Configuration

The example connects to `wss://api.anychat.io` and `https://api.anychat.io/api/v1` by default. To connect to your own server, modify the configuration in `src/app.ts`:

```typescript
const client = await createAnyChatClient({
  gatewayUrl: 'wss://your-server.com',
  apiBaseUrl: 'https://your-server.com/api/v1',
  deviceId: this.getOrCreateDeviceId(),
  // ... other options
}, '/lib/anychat.js');
```

## Project Structure

```
example/
├── public/           # Static assets
│   └── lib/          # WASM files (anychat.js, anychat.wasm)
├── src/
│   └── app.ts        # Main application code
├── index.html        # HTML entry point
├── package.json      # Dependencies and scripts
└── README.md         # This file
```

## Key Concepts

### Client Initialization

```typescript
const client = await createAnyChatClient(config, wasmModulePath);
```

The client must be initialized with the WASM module before use.

### Event Listeners

```typescript
client.on('messageReceived', (message) => {
  console.log('New message:', message);
});
```

The client emits various events for real-time updates.

### Promise-based API

```typescript
const conversations = await client.getConversationList();
await client.sendTextMessage(convId, 'Hello!');
```

All async operations return Promises for easy async/await usage.

## Learn More

- [SDK Documentation](../README.md)
- [API Reference](https://yzhgit.github.io/anychat-server)
- [Backend Repository](https://github.com/yzhgit/anychat-server)
