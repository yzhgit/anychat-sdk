import 'package:flutter/foundation.dart';
import 'package:anychat_sdk/anychat_sdk.dart';
import 'package:shared_preferences/shared_preferences.dart';

/// Authentication service managing SDK client and auth state
class AuthService extends ChangeNotifier {
  final String gatewayUrl;
  final String apiBaseUrl;
  final String deviceId;

  AnyChatClient? _client;
  AuthToken? _currentToken;
  ConnectionState _connectionState = ConnectionState.disconnected;
  String? _errorMessage;

  bool _isInitialized = false;

  AuthService({
    required this.gatewayUrl,
    required this.apiBaseUrl,
    required this.deviceId,
  });

  // Getters
  bool get isLoggedIn => _currentToken != null;
  bool get isConnected => _connectionState == ConnectionState.connected;
  bool get isInitialized => _isInitialized;
  ConnectionState get connectionState => _connectionState;
  String? get errorMessage => _errorMessage;
  AuthToken? get currentToken => _currentToken;
  String? get userId => _currentToken?.userId;

  /// Initialize SDK client and connect
  Future<void> initialize() async {
    if (_isInitialized) return;

    try {
      _errorMessage = null;

      // Create client
      _client = AnyChatClient(
        gatewayUrl: gatewayUrl,
        apiBaseUrl: apiBaseUrl,
        deviceId: deviceId,
        dbPath: null, // In-memory for sample
      );

      // Listen to connection state changes
      _client!.connectionStateStream.listen((state) {
        _connectionState = state;
        notifyListeners();
      });

      // Connect to gateway
      _client!.connect();

      _isInitialized = true;

      // Try to restore session from shared preferences
      await _restoreSession();

      notifyListeners();
    } catch (e) {
      _errorMessage = 'Failed to initialize SDK: $e';
      notifyListeners();
      rethrow;
    }
  }

  /// Register a new user account
  Future<void> register({
    required String email,
    required String password,
    required String username,
    String? avatar,
  }) async {
    if (_client == null) {
      throw Exception('SDK not initialized. Call initialize() first.');
    }

    try {
      _errorMessage = null;
      notifyListeners();

      final token = await _client!.register(
        account: email,
        password: password,
        username: username,
        avatar: avatar,
      );

      _currentToken = token;
      await _saveSession(token);
      notifyListeners();
    } catch (e) {
      _errorMessage = 'Registration failed: $e';
      notifyListeners();
      rethrow;
    }
  }

  /// Login with existing account
  Future<void> login({
    required String account,
    required String password,
  }) async {
    if (_client == null) {
      throw Exception('SDK not initialized. Call initialize() first.');
    }

    try {
      _errorMessage = null;
      notifyListeners();

      final token = await _client!.login(
        account: account,
        password: password,
      );

      _currentToken = token;
      await _saveSession(token);
      notifyListeners();
    } catch (e) {
      _errorMessage = 'Login failed: $e';
      notifyListeners();
      rethrow;
    }
  }

  /// Logout and clear session
  Future<void> logout() async {
    if (_client == null) return;

    try {
      _errorMessage = null;

      await _client!.logout();
      _currentToken = null;
      await _clearSession();
      notifyListeners();
    } catch (e) {
      _errorMessage = 'Logout failed: $e';
      notifyListeners();
      rethrow;
    }
  }

  /// Save session to shared preferences
  Future<void> _saveSession(AuthToken token) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('access_token', token.accessToken);
    await prefs.setString('refresh_token', token.refreshToken);
    await prefs.setInt('expires_at_ms', token.expiresAtMs);
    if (token.userId != null) {
      await prefs.setString('user_id', token.userId!);
    }
  }

  /// Restore session from shared preferences
  Future<void> _restoreSession() async {
    final prefs = await SharedPreferences.getInstance();
    final accessToken = prefs.getString('access_token');
    final refreshToken = prefs.getString('refresh_token');
    final expiresAtMs = prefs.getInt('expires_at_ms');
    final userId = prefs.getString('user_id');

    if (accessToken != null && refreshToken != null && expiresAtMs != null) {
      // Check if token is expired
      if (DateTime.now().millisecondsSinceEpoch < expiresAtMs) {
        _currentToken = AuthToken(
          accessToken: accessToken,
          refreshToken: refreshToken,
          expiresAtMs: expiresAtMs,
          userId: userId,
        );
        notifyListeners();
      } else {
        // Token expired, clear session
        await _clearSession();
      }
    }
  }

  /// Clear session from shared preferences
  Future<void> _clearSession() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove('access_token');
    await prefs.remove('refresh_token');
    await prefs.remove('expires_at_ms');
    await prefs.remove('user_id');
  }

  @override
  void dispose() {
    _client?.dispose();
    super.dispose();
  }
}
