# AnyChatSDK Example App

This is a complete example iOS application demonstrating the usage of AnyChatSDK.

## Features Demonstrated

- User authentication (login/logout)
- Connection state monitoring
- Conversation list display
- Real-time messaging
- Friend list management
- Group list display
- User profile display

## Setup

1. Install CocoaPods dependencies:

```bash
pod install
```

2. Open the workspace:

```bash
open AnyChatSDKExample.xcworkspace
```

3. Configure the server URL in `ContentView.swift`:

```swift
let config = ClientConfig(
    gatewayURL: "wss://your-server.com",  // Update this
    apiBaseURL: "https://your-server.com/api/v1",  // Update this
    deviceId: UIDevice.current.identifierForVendor?.uuidString ?? "unknown",
    dbPath: documentsPath + "/anychat_example.db"
)
```

4. Run the app in Xcode (Cmd+R)

## Requirements

- iOS 13.0+
- Xcode 15.0+
- CocoaPods 1.10+

## Usage

### Login

Enter your account credentials on the login screen. The app will automatically connect to the server and authenticate.

### Conversations

The Chats tab shows all your conversations. Tap on a conversation to view messages and send new ones.

### Friends

The Friends tab displays your friend list. You can refresh the list using the refresh button in the navigation bar.

### Groups

The Groups tab shows all groups you're a member of.

### Profile

The Profile tab displays your user information and connection status. Use the logout button to sign out.

## Code Structure

- `ContentView.swift` - Main app structure with SwiftUI views
- `AppState` - Observable object managing client state
- `LoginView` - Authentication screen
- `ConversationsView` - Conversation list
- `ChatView` - Individual chat screen
- `FriendsView` - Friend list
- `GroupsView` - Group list
- `ProfileView` - User profile

## Architecture

The example app uses:

- SwiftUI for the user interface
- Swift concurrency (async/await, Task)
- ObservableObject for state management
- EnvironmentObject for dependency injection

## Customization

Feel free to modify this example to suit your needs. You can:

- Add custom UI styling
- Implement additional features (file upload, voice/video calls, etc.)
- Add push notifications
- Implement offline message caching
- Add message search functionality

## Support

If you encounter any issues:

1. Check the [main README](../README.md) for SDK documentation
2. Verify your server URL is correct
3. Check console logs for error messages
4. Open an issue on [GitHub](https://github.com/yzhgit/anychat-sdk/issues)

## License

This example app is provided under the same MIT license as AnyChatSDK.
