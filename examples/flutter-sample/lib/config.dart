/// Application configuration
class AppConfig {
  // Backend API endpoints
  // Change these to match your AnyChat server deployment
  static const String gatewayUrl = 'wss://api.anychat.io';
  static const String apiBaseUrl = 'https://api.anychat.io/api/v1';

  // Device identification
  static const String deviceId = 'flutter-desktop-sample';

  // Optional: Local database path (null for in-memory)
  static const String? dbPath = null;

  // Connection settings
  static const int connectTimeoutMs = 10000;
  static const int maxReconnectAttempts = 5;
  static const bool autoReconnect = true;
}
