# AnyChat Web SDK Example

This is a complete example application demonstrating the AnyChat Web SDK.

## Features Demonstrated

- User authentication (login/register)
- Real-time connection status
- Conversation list management
- Message sending and receiving
- Message history loading
- Unread count tracking
- Event-driven architecture

## Running the Example

### Prerequisites

1. Build the Web SDK first:
   ```bash
   cd ..
   ./build.sh
   ```

2. Start a local web server (WASM requires proper MIME types):
   ```bash
   # Using Python
   python3 -m http.server 8000

   # Or using Node.js
   npx http-server -p 8000

   # Or using PHP
   php -S localhost:8000
   ```

3. Open http://localhost:8000 in your browser

## Code Structure

- `index.html` - Main HTML page with UI layout
- `app.ts` - TypeScript application code
- `app.js` - Compiled JavaScript (generated from app.ts)

## Adapting for Production

### 1. Update API Endpoints

In `app.ts`, change the URLs:

```typescript
const client = await createAnyChatClient({
  gatewayUrl: 'wss://your-api.example.com',  // Your WebSocket gateway
  apiBaseUrl: 'https://your-api.example.com/api/v1',  // Your HTTP API
  deviceId: this.getOrCreateDeviceId(),
});
```

### 2. Implement Proper Authentication

- Store tokens securely (consider using HttpOnly cookies for production)
- Implement token refresh logic
- Handle auth expiration gracefully

### 3. Add Error Handling

- Network errors
- Connection failures
- API errors
- Validation errors

### 4. Optimize Performance

- Implement message pagination
- Add virtual scrolling for large message lists
- Cache conversation data
- Debounce frequent operations

### 5. Enhance UI/UX

- Add loading states
- Implement retry mechanisms
- Show typing indicators
- Add read receipts
- Implement message reactions

### 6. Security Best Practices

- Validate all user input
- Sanitize message content (prevent XSS)
- Use HTTPS/WSS in production
- Implement rate limiting
- Add CSRF protection

## Browser Compatibility

This example works in all modern browsers that support:
- WebAssembly
- ES6 modules
- Async/await
- Fetch API

Tested on:
- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

## Building for Production

For production deployment:

1. Compile TypeScript:
   ```bash
   tsc app.ts --target ES2020 --module ES2020
   ```

2. Bundle with your preferred bundler (Webpack, Rollup, Vite, etc.)

3. Optimize WASM loading:
   - Use streaming compilation
   - Enable compression (gzip/brotli)
   - Implement proper caching headers

4. Consider code splitting to load the WASM module on-demand

## Troubleshooting

### WASM fails to load

- Ensure your server serves `.wasm` files with `application/wasm` MIME type
- Check browser console for CORS errors
- Verify the WASM file path is correct

### Connection issues

- Check that the WebSocket URL uses `wss://` (not `ws://`) in production
- Verify firewall/proxy settings
- Check browser network tab for connection errors

### Messages not appearing

- Verify connection state is "Connected"
- Check that you're subscribed to the correct conversation
- Look for errors in the browser console

## Next Steps

- Implement file/image upload
- Add voice/video call features
- Implement group chat features
- Add notification support
- Implement offline message queue
- Add service worker for offline support

## License

MIT
