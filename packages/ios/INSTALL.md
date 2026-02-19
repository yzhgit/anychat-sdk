# AnyChatSDK for iOS/macOS - Quick Installation

Get started with AnyChatSDK in minutes.

## Installation

### Option 1: CocoaPods (Recommended)

1. Add to your `Podfile`:

```ruby
platform :ios, '13.0'
use_frameworks!

target 'YourApp' do
  pod 'AnyChatSDK', '~> 0.1'
end
```

2. Install:

```bash
pod install
```

3. Open the `.xcworkspace` file:

```bash
open YourApp.xcworkspace
```

### Option 2: Swift Package Manager

Add via Xcode:

1. File → Add Packages...
2. Enter: `https://github.com/yzhgit/anychat-sdk.git`
3. Select version: `0.1.0` or later

Or add to `Package.swift`:

```swift
dependencies: [
    .package(url: "https://github.com/yzhgit/anychat-sdk.git", from: "0.1.0")
]
```

## Quick Start

### 1. Import the SDK

```swift
import AnyChatSDK
```

### 2. Configure the Client

```swift
let config = ClientConfig(
    gatewayURL: "wss://api.anychat.io",
    apiBaseURL: "https://api.anychat.io/api/v1",
    deviceId: UIDevice.current.identifierForVendor?.uuidString ?? "unknown",
    dbPath: NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] + "/anychat.db"
)

let client = try AnyChatClient(config: config)
client.connect()
```

### 3. Login

```swift
let token = try await client.auth.login(
    account: "user@example.com",
    password: "password"
)
print("Logged in: \(token.accessToken)")
```

### 4. Send a Message

```swift
try await client.message.sendText(
    sessionId: "conv_123",
    content: "Hello, World!"
)
```

### 5. Listen for Messages

```swift
Task {
    for await message in client.message.receivedMessages {
        print("New message: \(message.content)")
    }
}
```

## Example App

See the complete example app in the [Example](Example/) directory.

Run it:

```bash
cd Example
pod install
open AnyChatSDKExample.xcworkspace
```

## Documentation

- [Full README](README.md) - Complete feature documentation
- [Example App](Example/README.md) - Example app guide
- [Publishing Guide](PUBLISHING.md) - For maintainers
- [API Documentation](https://yzhgit.github.io/anychat-server)

## Requirements

- iOS 13.0+ / macOS 10.15+
- Swift 5.9+
- Xcode 15.0+

## Support

- **Issues**: [GitHub](https://github.com/yzhgit/anychat-sdk/issues)
- **Email**: sdk@anychat.io
