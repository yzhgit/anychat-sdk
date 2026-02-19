# Quick Reference Guide

## Installation

```yaml
dependencies:
  anychat_sdk: ^0.1.0
  path_provider: ^2.0.0
```

```bash
flutter pub get
```

## Basic Setup

```dart
import 'package:anychat_sdk/anychat_sdk.dart';
import 'package:path_provider/path_provider.dart';

// Initialize
final appDir = await getApplicationSupportDirectory();
final client = AnyChatClient(
  gatewayUrl: 'wss://api.anychat.io',
  apiBaseUrl: 'https://api.anychat.io/api/v1',
  deviceId: 'unique-device-id',
  dbPath: '${appDir.path}/anychat.db',
);

// Connect
client.connect();

// Listen to events
client.connectionStateStream.listen((state) => print('State: $state'));
client.messageReceivedStream.listen((msg) => print('Message: ${msg.content}'));
```

## Core Operations

### Authentication

```dart
// Login
final token = await client.login(
  account: 'user@example.com',
  password: 'password',
);

// Check login status
if (client.isLoggedIn) {
  // User is logged in
}

// Get current token
final currentToken = client.currentToken;

// Logout
await client.logout();
```

### Messaging

```dart
// Send text message
await client.sendTextMessage(
  sessionId: 'conv-123',
  content: 'Hello!',
);

// Get message history
final messages = await client.getMessageHistory(
  sessionId: 'conv-123',
  limit: 20,
);
```

### Conversations

```dart
// Get all conversations
final conversations = await client.getConversations();

// Mark as read
await client.markConversationRead('conv-123');
```

### Friends

```dart
// Get friend list
final friends = await client.getFriends();
```

## Event Streams

```dart
// Connection state changes
client.connectionStateStream.listen((state) {
  // ConnectionState.disconnected
  // ConnectionState.connecting
  // ConnectionState.connected
  // ConnectionState.reconnecting
});

// New messages
client.messageReceivedStream.listen((message) {
  // Handle incoming message
});

// Conversation updates
client.conversationUpdatedStream.listen((conversation) {
  // Handle conversation update
});

// Friend requests
client.friendRequestStream.listen((request) {
  // Handle friend request
});

// Incoming calls
client.incomingCallStream.listen((call) {
  // Handle incoming call
});
```

## Connection Management

```dart
// Connect to server
client.connect();

// Disconnect
client.disconnect();

// Get current connection state
final state = client.connectionState;

// Clean up when done
client.dispose();
```

## Data Models

### Message

```dart
class Message {
  final String messageId;
  final String localId;
  final String convId;
  final String senderId;
  final MessageType type;  // text, image, file, audio, video
  final String content;
  final int timestampMs;
  final SendState sendState;  // pending, sent, failed
  final bool isRead;
}
```

### Conversation

```dart
class Conversation {
  final String convId;
  final ConversationType convType;  // private_, group
  final String targetId;
  final String lastMsgText;
  final int lastMsgTimeMs;
  final int unreadCount;
  final bool isPinned;
  final bool isMuted;
}
```

### Friend

```dart
class Friend {
  final String userId;
  final String remark;
  final UserInfo userInfo;
}
```

### AuthToken

```dart
class AuthToken {
  final String accessToken;
  final String refreshToken;
  final int expiresAtMs;

  bool get isExpired => DateTime.now().millisecondsSinceEpoch > expiresAtMs;
}
```

## Platform Configuration

### Android
- Min SDK: 21
- No additional configuration needed

### iOS
- Deployment target: 12.0
- No additional configuration needed

### macOS
- Deployment target: 10.14
- Add entitlements for network access:

```xml
<key>com.apple.security.network.client</key>
<true/>
```

### Linux
- Install dependencies:
```bash
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

### Windows
- No additional configuration needed

## Error Handling

```dart
try {
  final token = await client.login(
    account: 'user@example.com',
    password: 'password',
  );
} catch (e) {
  print('Login failed: $e');
}
```

## Common Patterns

### Initialize and Login

```dart
class MyApp extends StatefulWidget {
  @override
  _MyAppState createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  late AnyChatClient client;

  @override
  void initState() {
    super.initState();
    _initClient();
  }

  Future<void> _initClient() async {
    final appDir = await getApplicationSupportDirectory();
    client = AnyChatClient(
      gatewayUrl: 'wss://api.anychat.io',
      apiBaseUrl: 'https://api.anychat.io/api/v1',
      deviceId: 'device-id',
      dbPath: '${appDir.path}/anychat.db',
    );

    client.connectionStateStream.listen((state) {
      setState(() {
        // Update UI
      });
    });

    client.connect();
  }

  @override
  void dispose() {
    client.dispose();
    super.dispose();
  }
}
```

### Handle Messages

```dart
void _setupMessageListener() {
  client.messageReceivedStream.listen((message) {
    if (message.convId == currentConversationId) {
      setState(() {
        messages.add(message);
      });
    }
  });
}
```

### Send Message with Error Handling

```dart
Future<void> _sendMessage(String content) async {
  try {
    await client.sendTextMessage(
      sessionId: conversationId,
      content: content,
    );
  } catch (e) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Failed to send: $e')),
    );
  }
}
```

## Links

- [Full Documentation](README.md)
- [Example App](example/)
- [Publishing Guide](PUBLISHING.md)
- [Package Structure](PACKAGE_STRUCTURE.md)
- [GitHub](https://github.com/yzhgit/anychat-sdk)
- [Backend API](https://yzhgit.github.io/anychat-server)
