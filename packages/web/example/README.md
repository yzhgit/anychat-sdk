# AnyChat Web SDK Example

A modern web application demonstrating the AnyChat SDK integration using TypeScript and Vite.

## Features

- 🔐 User authentication (login)
- 💬 Real-time messaging
- 📋 Conversation list
- 🔄 Connection status monitoring
- 💾 Local storage for device ID and token
- 🎨 Modern, responsive UI

## Quick Start

```bash
# Install dependencies
npm install

# Start development server
npm run dev

# Build for production
npm run build
```

## Configuration

Update server URLs in `src/app.ts`:

```typescript
this.client = await createAnyChatClient({
  gatewayUrl: 'wss://your-server.com',
  apiBaseUrl: 'https://your-server.com/api/v1',
  // ...
});
```

## Project Structure

```
example/
├── index.html      # Main HTML with embedded CSS
├── src/
│   └── app.ts     # Application logic
├── package.json
└── vite.config.ts
```

## Features Demonstrated

1. **SDK Initialization** - Setup with WebAssembly
2. **Authentication** - Login with credentials
3. **Connection Management** - Real-time status
4. **Conversations** - List and select chats
5. **Messaging** - Send/receive messages
6. **Event Handling** - Connection, message, conversation events

## Development

The app uses vanilla TypeScript (no framework) for simplicity:

- **Vite** - Fast dev server with HMR
- **TypeScript** - Type safety
- **Modern CSS** - Gradient design
- **WebAssembly** - AnyChat SDK

## Troubleshooting

**Issue:** "Cannot find module '@anychat/sdk'"

**Solution:** Build the parent SDK first:
```bash
cd packages/web && npm install && npm run build
```

**Issue:** Connection timeout

**Solutions:**
- Verify server URLs
- Check if WebAssembly is supported
- Ensure CORS is configured

## License

MIT License - see [../../LICENSE](../../LICENSE)

## Links

- [SDK Documentation](../README.md)
- [Backend API](https://github.com/yzhgit/anychat-server)
- [Issues](https://github.com/yzhgit/anychat-sdk/issues)
