# AnyChat SDK Example

This example demonstrates how to use the AnyChat SDK for Flutter.

## Features Demonstrated

- Client initialization with configuration
- Connection management (connect/disconnect)
- Authentication (login/logout)
- Connection state monitoring
- Real-time message receiving
- Conversation list retrieval
- Friend list retrieval
- Event stream handling

## Running the Example

1. Make sure you have Flutter installed and set up.

2. Get dependencies:
   ```bash
   cd packages/flutter/example
   flutter pub get
   ```

3. Update the server URLs in the app if needed (default: `wss://api.anychat.io`).

4. Run the app:
   ```bash
   # For Android
   flutter run

   # For iOS
   flutter run -d ios

   # For desktop
   flutter run -d windows
   flutter run -d linux
   flutter run -d macos
   ```

## Using the Example

1. **Connect**: Tap "Connect" to establish a WebSocket connection to the server.

2. **Login**: Enter your account and password, then tap "Login".

3. **Monitor Status**: Watch the connection state and login status at the top.

4. **Get Data**: Use "Get Conversations" and "Get Friends" to fetch data.

5. **View Logs**: All events and operations are logged at the bottom.

6. **Clean Up**: Tap "Disconnect" or "Logout" when done.

## Notes

- The example uses placeholder server URLs. Update them to point to your actual AnyChat server.
- Make sure your server is running and accessible from your device/emulator.
- The device ID is generated randomly each time. In a production app, you should persist it.
- Some async operations may show "UnimplementedError" as they require additional native callback support.

## Troubleshooting

If you encounter build errors:

```bash
flutter clean
flutter pub get
flutter run
```

If connection fails:
- Verify the server URL is correct and accessible
- Check if the server is running
- Ensure your device has internet connectivity
