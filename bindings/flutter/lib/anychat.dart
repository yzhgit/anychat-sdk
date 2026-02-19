/// AnyChat IM SDK for Flutter
///
/// Provides a native FFI-based SDK for building cross-platform IM applications
/// on Android, iOS, Windows, Linux, and macOS.
///
/// Usage:
/// ```dart
/// import 'package:anychat_flutter/anychat.dart';
///
/// final client = AnyChatClient(
///   gatewayUrl: 'wss://api.anychat.io',
///   apiBaseUrl: 'https://api.anychat.io/api/v1',
///   deviceId: '<unique-device-id>',
///   dbPath: '<path-to-local-db>',
/// );
///
/// client.connect();
///
/// // Listen to connection state
/// client.connectionStateStream.listen((state) {
///   print('Connection state: $state');
/// });
///
/// // Login
/// final token = await client.login(
///   account: 'user@example.com',
///   password: 'password',
/// );
///
/// // Send a message
/// await client.sendTextMessage(
///   sessionId: 'conv-123',
///   content: 'Hello!',
/// );
///
/// // Clean up
/// client.dispose();
/// ```
library anychat;

export 'src/anychat_client.dart';
export 'src/models.dart';
