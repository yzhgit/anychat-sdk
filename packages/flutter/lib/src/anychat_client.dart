import 'dart:async';
import 'dart:ffi';
import 'package:ffi/ffi.dart';

import 'anychat_ffi_bindings.dart';
import 'native_loader.dart';
import 'models.dart';

/// Main AnyChat client for Flutter.
///
/// This is a high-level Dart wrapper around the C FFI bindings.
/// All async operations return Future<T> instead of C callbacks.
class AnyChatClient {
  late final AnyChatNativeBindings _bindings;
  late final Pointer<AnyChatClient_T> _clientHandle;

  // Sub-module handles (cached)
  Pointer<AnyChatAuthManager_T>? _authHandle;
  Pointer<AnyChatMessage_T>? _messageHandle;
  Pointer<AnyChatConversation_T>? _convHandle;
  Pointer<AnyChatFriend_T>? _friendHandle;
  Pointer<AnyChatGroup_T>? _groupHandle;
  Pointer<AnyChatFile_T>? _fileHandle;
  Pointer<AnyChatUser_T>? _userHandle;
  Pointer<AnyChatRtc_T>? _rtcHandle;

  // Connection state stream
  final _connectionStateController = StreamController<ConnectionState>.broadcast();
  Stream<ConnectionState> get connectionStateStream => _connectionStateController.stream;

  // Message received stream
  final _messageReceivedController = StreamController<Message>.broadcast();
  Stream<Message> get messageReceivedStream => _messageReceivedController.stream;

  // Conversation updated stream
  final _conversationUpdatedController = StreamController<Conversation>.broadcast();
  Stream<Conversation> get conversationUpdatedStream => _conversationUpdatedController.stream;

  // Friend request stream
  final _friendRequestController = StreamController<FriendRequest>.broadcast();
  Stream<FriendRequest> get friendRequestStream => _friendRequestController.stream;

  // Incoming call stream
  final _incomingCallController = StreamController<CallSession>.broadcast();
  Stream<CallSession> get incomingCallStream => _incomingCallController.stream;

  /// Creates a new AnyChat client with the given configuration.
  AnyChatClient({
    required String gatewayUrl,
    required String apiBaseUrl,
    required String deviceId,
    String? dbPath,
    int connectTimeoutMs = 10000,
    int maxReconnectAttempts = 5,
    bool autoReconnect = true,
  }) {
    _bindings = AnyChatNativeBindings(NativeLibraryLoader.library);

    // Allocate and populate the config struct
    final config = calloc<AnyChatClientConfig_C>();
    config.ref.gateway_url = gatewayUrl.toNativeUtf8().cast();
    config.ref.api_base_url = apiBaseUrl.toNativeUtf8().cast();
    config.ref.device_id = deviceId.toNativeUtf8().cast();
    config.ref.db_path = dbPath != null ? dbPath.toNativeUtf8().cast() : nullptr;
    config.ref.connect_timeout_ms = connectTimeoutMs;
    config.ref.max_reconnect_attempts = maxReconnectAttempts;
    config.ref.auto_reconnect = autoReconnect ? 1 : 0;

    // Create the client
    _clientHandle = _bindings.anychat_client_create(config);

    // Free the config strings
    calloc.free(config.ref.gateway_url);
    calloc.free(config.ref.api_base_url);
    calloc.free(config.ref.device_id);
    if (dbPath != null) calloc.free(config.ref.db_path);
    calloc.free(config);

    if (_clientHandle == nullptr) {
      final errorPtr = _bindings.anychat_get_last_error();
      final error = errorPtr.cast<Utf8>().toDartString();
      throw Exception('Failed to create client: $error');
    }

    // Setup connection state callback
    _setupConnectionStateCallback();
  }

  void _setupConnectionStateCallback() {
    // TODO: Implement native callback registration via Dart FFI
    // This requires using NativeCallable (Dart 2.18+) to create a C function pointer
    // from a Dart function. For now, polling can be used as a workaround.
  }

  /// Connects to the server.
  void connect() {
    _bindings.anychat_client_connect(_clientHandle);
  }

  /// Disconnects from the server.
  void disconnect() {
    _bindings.anychat_client_disconnect(_clientHandle);
  }

  /// Gets the current connection state.
  ConnectionState get connectionState {
    final state = _bindings.anychat_client_get_connection_state(_clientHandle);
    return ConnectionState.fromInt(state);
  }

  /// Disposes the client and releases all resources.
  void dispose() {
    _bindings.anychat_client_disconnect(_clientHandle);
    _bindings.anychat_client_destroy(_clientHandle);
    _connectionStateController.close();
    _messageReceivedController.close();
    _conversationUpdatedController.close();
    _friendRequestController.close();
    _incomingCallController.close();
  }

  // ── Auth module ──────────────────────────────────────────────────────────────

  Pointer<AnyChatAuthManager_T> get _auth {
    _authHandle ??= _bindings.anychat_client_get_auth(_clientHandle);
    return _authHandle!;
  }

  /// Logs in with account and password.
  Future<AuthToken> login({
    required String account,
    required String password,
    String deviceType = 'flutter',
  }) {
    final completer = Completer<AuthToken>();

    // TODO: Implement using NativeCallable for callback
    // For now, this is a placeholder structure showing the intended API

    final accountPtr = account.toNativeUtf8();
    final passwordPtr = password.toNativeUtf8();
    final deviceTypePtr = deviceType.toNativeUtf8();

    // This will require a C callback wrapper
    // final ret = _bindings.anychat_auth_login(
    //   _auth,
    //   accountPtr.cast(),
    //   passwordPtr.cast(),
    //   deviceTypePtr.cast(),
    //   nullptr, // userdata
    //   nullptr, // callback (to be implemented)
    // );

    calloc.free(accountPtr);
    calloc.free(passwordPtr);
    calloc.free(deviceTypePtr);

    // Placeholder: complete with error
    completer.completeError(
      UnimplementedError('Async callbacks require NativeCallable (Dart 2.18+)')
    );

    return completer.future;
  }

  /// Checks if the user is logged in.
  bool get isLoggedIn {
    return _bindings.anychat_auth_is_logged_in(_auth) != 0;
  }

  /// Gets the current auth token (if logged in).
  AuthToken? get currentToken {
    if (!isLoggedIn) return null;

    final tokenStruct = calloc<AnyChatAuthToken_C>();
    final ret = _bindings.anychat_auth_get_current_token(_auth, tokenStruct);

    if (ret != 0) {
      calloc.free(tokenStruct);
      return null;
    }

    final token = AuthToken(
      accessToken: _copyFixedString(tokenStruct.ref.access_token, 512),
      refreshToken: _copyFixedString(tokenStruct.ref.refresh_token, 512),
      expiresAtMs: tokenStruct.ref.expires_at_ms,
    );

    calloc.free(tokenStruct);
    return token;
  }

  /// Logs out the current user.
  Future<void> logout() {
    final completer = Completer<void>();
    // TODO: Implement with NativeCallable
    completer.completeError(
      UnimplementedError('Async callbacks require NativeCallable')
    );
    return completer.future;
  }

  // ── Message module ───────────────────────────────────────────────────────────

  Pointer<AnyChatMessage_T> get _message {
    _messageHandle ??= _bindings.anychat_client_get_message(_clientHandle);
    return _messageHandle!;
  }

  /// Sends a text message to a conversation.
  Future<void> sendTextMessage({
    required String sessionId,
    required String content,
  }) {
    final completer = Completer<void>();
    // TODO: Implement with NativeCallable
    completer.completeError(
      UnimplementedError('Async callbacks require NativeCallable')
    );
    return completer.future;
  }

  /// Fetches message history for a conversation.
  Future<List<Message>> getMessageHistory({
    required String sessionId,
    int beforeTimestampMs = 0,
    int limit = 20,
  }) {
    final completer = Completer<List<Message>>();
    // TODO: Implement with NativeCallable
    completer.completeError(
      UnimplementedError('Async callbacks require NativeCallable')
    );
    return completer.future;
  }

  // ── Conversation module ──────────────────────────────────────────────────────

  Pointer<AnyChatConversation_T> get _conv {
    _convHandle ??= _bindings.anychat_client_get_conversation(_clientHandle);
    return _convHandle!;
  }

  /// Gets the list of conversations.
  Future<List<Conversation>> getConversations() {
    final completer = Completer<List<Conversation>>();
    // TODO: Implement with NativeCallable
    completer.completeError(
      UnimplementedError('Async callbacks require NativeCallable')
    );
    return completer.future;
  }

  /// Marks a conversation as read.
  Future<void> markConversationRead(String convId) {
    final completer = Completer<void>();
    // TODO: Implement with NativeCallable
    completer.completeError(
      UnimplementedError('Async callbacks require NativeCallable')
    );
    return completer.future;
  }

  // ── Friend module ────────────────────────────────────────────────────────────

  Pointer<AnyChatFriend_T> get _friend {
    _friendHandle ??= _bindings.anychat_client_get_friend(_clientHandle);
    return _friendHandle!;
  }

  /// Gets the friend list.
  Future<List<Friend>> getFriends() {
    final completer = Completer<List<Friend>>();
    // TODO: Implement with NativeCallable
    completer.completeError(
      UnimplementedError('Async callbacks require NativeCallable')
    );
    return completer.future;
  }

  // ── Helper methods ───────────────────────────────────────────────────────────

  /// Copies a fixed-size C string array to a Dart String.
  String _copyFixedString(Array<Char> arr, int maxLength) {
    final ptr = Pointer<Char>.fromAddress(arr.address);
    final units = <int>[];
    for (var i = 0; i < maxLength; i++) {
      final char = ptr.elementAt(i).value;
      if (char == 0) break;
      units.add(char);
    }
    return String.fromCharCodes(units);
  }
}
