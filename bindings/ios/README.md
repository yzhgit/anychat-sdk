# AnyChatSDK for iOS/macOS

Complete Swift wrapper for the AnyChat IM system C API, providing messaging, voice/video calls, group management, and real-time communication features.

## Features

- Modern Swift API with async/await
- AsyncStream-based event handling
- Full type safety with Swift structs
- Automatic memory management
- Thread-safe actor-based design
- Support for iOS 13+ and macOS 10.15+

## Installation

### Swift Package Manager

Add to your `Package.swift`:

```swift
dependencies: [
    .package(url: "https://github.com/yzhgit/anychat-sdk.git", from: "1.0.0")
]
```

### CocoaPods

Add to your `Podfile`:

```ruby
pod 'AnyChatSDK', '~> 1.0'
```

## Quick Start

### Initialize the Client

```swift
import AnyChatSDK

let config = ClientConfig(
    gatewayURL: "wss://api.anychat.io",
    apiBaseURL: "https://api.anychat.io/api/v1",
    deviceId: UIDevice.current.identifierForVendor?.uuidString ?? "unknown",
    dbPath: NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] + "/anychat.db"
)

let client = try AnyChatClient(config: config)
```

### Authentication

```swift
// Login
do {
    let token = try await client.auth.login(
        account: "user@example.com",
        password: "password123"
    )
    print("Logged in with token: \(token.accessToken)")
} catch {
    print("Login failed: \(error)")
}

// Listen for token expiration
Task {
    for await _ in client.auth.tokenExpired {
        print("Token expired, need to re-authenticate")
    }
}
```

### Messaging

```swift
// Send a text message
try await client.message.sendText(
    sessionId: "conv_123",
    content: "Hello, World!"
)

// Get message history
let messages = try await client.message.getHistory(
    sessionId: "conv_123",
    limit: 50
)

// Listen for incoming messages
Task {
    for await message in client.message.receivedMessages {
        print("New message from \(message.senderId): \(message.content)")
    }
}
```

### Conversations

```swift
// Get conversation list
let conversations = try await client.conversation.getList()

// Mark as read
try await client.conversation.markRead(conversationId: "conv_123")

// Pin/unpin conversation
try await client.conversation.setPinned(conversationId: "conv_123", pinned: true)

// Listen for conversation updates
Task {
    for await conversation in client.conversation.conversationUpdated {
        print("Conversation updated: \(conversation.conversationId)")
    }
}
```

### Friends

```swift
// Get friend list
let friends = try await client.friend.getList()

// Send friend request
try await client.friend.sendRequest(
    toUserId: "user_456",
    message: "Let's be friends!"
)

// Handle friend request
try await client.friend.handleRequest(
    requestId: 123,
    accept: true
)

// Listen for incoming friend requests
Task {
    for await request in client.friend.requestReceived {
        print("Friend request from: \(request.fromUserInfo.username)")
    }
}
```

### Groups

```swift
// Get group list
let groups = try await client.group.getList()

// Create a group
try await client.group.create(
    name: "My Group",
    memberIds: ["user_1", "user_2", "user_3"]
)

// Listen for group invitations
Task {
    for await (group, inviterId) in client.group.invited {
        print("Invited to group \(group.name) by \(inviterId)")
    }
}
```

### Voice/Video Calls

```swift
// Initiate a call
let session = try await client.rtc.initiateCall(
    calleeId: "user_456",
    callType: .video
)

// Join incoming call
try await client.rtc.joinCall(callId: session.callId)

// End call
try await client.rtc.endCall(callId: session.callId)

// Listen for incoming calls
Task {
    for await session in client.rtc.incomingCall {
        print("Incoming \(session.callType == .video ? "video" : "audio") call from \(session.callerId)")
    }
}

// Listen for call status changes
Task {
    for await (callId, status) in client.rtc.callStatusChanged {
        print("Call \(callId) status: \(status)")
    }
}
```

### Meetings

```swift
// Create a meeting
let meeting = try await client.rtc.createMeeting(
    title: "Team Meeting",
    password: "secret123",
    maxParticipants: 50
)

// Join a meeting
let room = try await client.rtc.joinMeeting(
    roomId: meeting.roomId,
    password: "secret123"
)

// End meeting
try await client.rtc.endMeeting(roomId: meeting.roomId)
```

### User Profile

```swift
// Get current user profile
let profile = try await client.user.getProfile()

// Update profile
var updatedProfile = profile
updatedProfile.nickname = "New Nickname"
try await client.user.updateProfile(updatedProfile)

// Search users
let (users, total) = try await client.user.search(
    keyword: "john",
    page: 1,
    pageSize: 20
)
```

### File Upload/Download

```swift
// Upload a file
let fileInfo = try await client.file.upload(
    localPath: "/path/to/image.jpg",
    fileType: "image"
) { uploaded, total in
    let progress = Double(uploaded) / Double(total) * 100
    print("Upload progress: \(progress)%")
}

// Get download URL
let downloadURL = try await client.file.getDownloadURL(fileId: fileInfo.fileId)

// Delete file
try await client.file.delete(fileId: fileInfo.fileId)
```

### Connection State

```swift
// Connect to server
client.connect()

// Listen for connection state changes
Task {
    for await state in client.connectionState {
        switch state {
        case .disconnected:
            print("Disconnected")
        case .connecting:
            print("Connecting...")
        case .connected:
            print("Connected")
        case .reconnecting:
            print("Reconnecting...")
        }
    }
}

// Disconnect
client.disconnect()
```

## Error Handling

All async methods can throw `AnyChatError`:

```swift
do {
    try await client.auth.login(account: "user", password: "pass")
} catch let error as AnyChatError {
    switch error {
    case .auth:
        print("Authentication failed")
    case .network:
        print("Network error")
    case .timeout:
        print("Request timed out")
    case .notFound:
        print("Resource not found")
    case .tokenExpired:
        print("Token expired")
    default:
        print("Error: \(error)")
    }
}
```

## Architecture

The SDK uses modern Swift concurrency features:

- **Actor-based design**: All managers are actors, ensuring thread-safety
- **async/await**: All asynchronous operations use async/await
- **AsyncStream**: Event streams use AsyncStream for reactive programming
- **Automatic memory management**: C resources are automatically freed
- **Unmanaged context passing**: Safe callback context management

## Thread Safety

All public APIs are thread-safe. The SDK uses Swift actors to serialize access to shared state. Event streams can be consumed from any Task.

## Memory Management

The SDK automatically manages C resources:

- Client handle is wrapped in `ClientHandleWrapper` with automatic cleanup
- C strings are properly allocated and freed
- List structures are freed after conversion to Swift arrays
- Callback contexts use `Unmanaged` for proper retain/release semantics

## Requirements

- iOS 13.0+ / macOS 10.15+
- Swift 5.9+
- Xcode 15.0+

## License

MIT License - See LICENSE file for details

## Support

- Documentation: https://yzhgit.github.io/anychat-server
- Issues: https://github.com/yzhgit/anychat-sdk/issues
- Email: dev@anychat.io
