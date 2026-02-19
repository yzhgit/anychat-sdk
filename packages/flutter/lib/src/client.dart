import '../../bindings/flutter/lib/src/anychat_client.dart';

/// Public Flutter API for the AnyChat SDK.
class AnyChatClient {
  final ClientConfig config;
  AnyChatClientNative? _native;

  AnyChatClient({required this.config});

  Future<void> connect() async {
    _native = AnyChatClientNative();
    // TODO: delegate to native binding
    throw UnimplementedError('connect() not yet implemented');
  }

  void disconnect() {
    // TODO
  }
}

class ClientConfig {
  final String gatewayUrl;
  final String apiBaseUrl;
  final int connectTimeoutMs;
  final bool autoReconnect;

  const ClientConfig({
    required this.gatewayUrl,
    required this.apiBaseUrl,
    this.connectTimeoutMs = 10000,
    this.autoReconnect = true,
  });
}
