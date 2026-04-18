import 'dart:async';
import 'dart:ffi';

import 'package:ffi/ffi.dart';

import 'anychat_ffi_bindings.dart';
import 'models.dart';
import 'native_loader.dart';

class _CallbackContext {
  final Object value;

  _CallbackContext(this.value);
}

final _callbackContexts = <int, _CallbackContext>{};
int _nextCallbackId = 1;
AnyChatNativeBindings? _globalBindings;

int _registerCallback(Object value) {
  final id = _nextCallbackId++;
  _callbackContexts[id] = _CallbackContext(value);
  return id;
}

void _unregisterCallback(int id) {
  _callbackContexts.remove(id);
}

T? _getCallback<T>(int id) {
  final value = _callbackContexts[id]?.value;
  if (value is T) {
    return value;
  }
  return null;
}

String _errorFromPointer(Pointer<Char> error) {
  if (error == nullptr) {
    return 'Unknown error';
  }
  try {
    return error.cast<Utf8>().toDartString();
  } catch (_) {
    return 'Unknown error';
  }
}

String _dispatchErrorMessage(int code) {
  return 'Request dispatch failed (code: $code)';
}

String _copyFixedStringStatic(Array<Char> arr, int maxLength) {
  final units = <int>[];
  for (var i = 0; i < maxLength; i++) {
    final char = arr[i];
    if (char == 0) break;
    units.add(char);
  }
  return String.fromCharCodes(units);
}

void _authTokenSuccessNative(
  Pointer<Void> userdata,
  Pointer<AnyChatAuthToken_C> token,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<AuthToken>>(callbackId);
  if (completer == null) return;

  if (token == nullptr) {
    completer.completeError(Exception('Empty auth token'));
    _unregisterCallback(callbackId);
    return;
  }

  completer.complete(
    AuthToken(
      accessToken: _copyFixedStringStatic(token.ref.access_token, 512),
      refreshToken: _copyFixedStringStatic(token.ref.refresh_token, 512),
      expiresAtMs: token.ref.expires_at_ms,
    ),
  );
  _unregisterCallback(callbackId);
}

void _authTokenErrorNative(
  Pointer<Void> userdata,
  int code,
  Pointer<Char> error,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<AuthToken>>(callbackId);
  if (completer == null) return;
  completer.completeError(Exception(_errorFromPointer(error)));
  _unregisterCallback(callbackId);
}

void _voidSuccessNative(Pointer<Void> userdata) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<void>>(callbackId);
  if (completer == null) return;
  completer.complete();
  _unregisterCallback(callbackId);
}

void _voidErrorNative(
  Pointer<Void> userdata,
  int code,
  Pointer<Char> error,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<void>>(callbackId);
  if (completer == null) return;
  completer.completeError(Exception(_errorFromPointer(error)));
  _unregisterCallback(callbackId);
}

void _messageListSuccessNative(
  Pointer<Void> userdata,
  Pointer<AnyChatMessageList_C> list,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<List<Message>>>(callbackId);
  if (completer == null) return;

  final messages = <Message>[];
  if (list != nullptr) {
    final count = list.ref.count;
    final items = list.ref.items;
    if (items != nullptr) {
      for (var i = 0; i < count; i++) {
        final item = (items + i).ref;
        messages.add(
          Message(
            messageId: _copyFixedStringStatic(item.message_id, 64),
            localId: _copyFixedStringStatic(item.local_id, 64),
            convId: _copyFixedStringStatic(item.conv_id, 64),
            senderId: _copyFixedStringStatic(item.sender_id, 64),
            contentType: item.content_type,
            type: MessageType.fromInt(item.type),
            content: item.content == nullptr
                ? ''
                : item.content.cast<Utf8>().toDartString(),
            seq: item.seq,
            replyTo: _copyFixedStringStatic(item.reply_to, 64),
            timestampMs: item.timestamp_ms,
            status: item.status,
            sendState: SendState.fromInt(item.send_state),
            isRead: item.is_read != 0,
          ),
        );
      }
    }
    _globalBindings?.anychat_free_message_list(list);
  }

  completer.complete(messages);
  _unregisterCallback(callbackId);
}

void _messageListErrorNative(
  Pointer<Void> userdata,
  int code,
  Pointer<Char> error,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<List<Message>>>(callbackId);
  if (completer == null) return;
  completer.completeError(Exception(_errorFromPointer(error)));
  _unregisterCallback(callbackId);
}

void _convListSuccessNative(
  Pointer<Void> userdata,
  Pointer<AnyChatConversationList_C> list,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<List<Conversation>>>(callbackId);
  if (completer == null) return;

  final conversations = <Conversation>[];
  if (list != nullptr) {
    final count = list.ref.count;
    final items = list.ref.items;
    if (items != nullptr) {
      for (var i = 0; i < count; i++) {
        final item = (items + i).ref;
        conversations.add(
          Conversation(
            convId: _copyFixedStringStatic(item.conv_id, 64),
            convType: ConversationType.fromInt(item.conv_type),
            targetId: _copyFixedStringStatic(item.target_id, 64),
            lastMsgId: _copyFixedStringStatic(item.last_msg_id, 64),
            lastMsgText: _copyFixedStringStatic(item.last_msg_text, 512),
            lastMsgTimeMs: item.last_msg_time_ms,
            unreadCount: item.unread_count,
            isPinned: item.is_pinned != 0,
            isMuted: item.is_muted != 0,
            updatedAtMs: item.updated_at_ms,
          ),
        );
      }
    }
    _globalBindings?.anychat_free_conversation_list(list);
  }

  completer.complete(conversations);
  _unregisterCallback(callbackId);
}

void _convListErrorNative(
  Pointer<Void> userdata,
  int code,
  Pointer<Char> error,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<List<Conversation>>>(callbackId);
  if (completer == null) return;
  completer.completeError(Exception(_errorFromPointer(error)));
  _unregisterCallback(callbackId);
}

void _friendListSuccessNative(
  Pointer<Void> userdata,
  Pointer<AnyChatFriendList_C> list,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<List<Friend>>>(callbackId);
  if (completer == null) return;

  final friends = <Friend>[];
  if (list != nullptr) {
    final count = list.ref.count;
    final items = list.ref.items;
    if (items != nullptr) {
      for (var i = 0; i < count; i++) {
        final item = (items + i).ref;
        friends.add(
          Friend(
            userId: _copyFixedStringStatic(item.user_id, 64),
            remark: _copyFixedStringStatic(item.remark, 128),
            updatedAtMs: item.updated_at_ms,
            isDeleted: item.is_deleted != 0,
            userInfo: UserInfo(
              userId: _copyFixedStringStatic(item.user_info.user_id, 64),
              username: _copyFixedStringStatic(item.user_info.username, 128),
              avatarUrl: _copyFixedStringStatic(item.user_info.avatar_url, 512),
            ),
          ),
        );
      }
    }
    _globalBindings?.anychat_free_friend_list(list);
  }

  completer.complete(friends);
  _unregisterCallback(callbackId);
}

void _friendListErrorNative(
  Pointer<Void> userdata,
  int code,
  Pointer<Char> error,
) {
  final callbackId = userdata.address;
  final completer = _getCallback<Completer<List<Friend>>>(callbackId);
  if (completer == null) return;
  completer.completeError(Exception(_errorFromPointer(error)));
  _unregisterCallback(callbackId);
}

void _connectionStateCallbackNative(
  Pointer<Void> userdata,
  int state,
) {
  final client = _getCallback<AnyChatClient>(userdata.address);
  if (client == null) return;
  client._connectionStateController.add(ConnectionState.fromInt(state));
}

final _authTokenSuccessCallable = NativeCallable<
    Void Function(Pointer<Void>, Pointer<AnyChatAuthToken_C>)>.listener(
  _authTokenSuccessNative,
);
final _authTokenErrorCallable =
    NativeCallable<Void Function(Pointer<Void>, Int, Pointer<Char>)>.listener(
  _authTokenErrorNative,
);

final _voidSuccessCallable =
    NativeCallable<Void Function(Pointer<Void>)>.listener(
  _voidSuccessNative,
);
final _voidErrorCallable =
    NativeCallable<Void Function(Pointer<Void>, Int, Pointer<Char>)>.listener(
  _voidErrorNative,
);

final _messageListSuccessCallable = NativeCallable<
    Void Function(Pointer<Void>, Pointer<AnyChatMessageList_C>)>.listener(
  _messageListSuccessNative,
);
final _messageListErrorCallable =
    NativeCallable<Void Function(Pointer<Void>, Int, Pointer<Char>)>.listener(
  _messageListErrorNative,
);

final _convListSuccessCallable = NativeCallable<
    Void Function(Pointer<Void>, Pointer<AnyChatConversationList_C>)>.listener(
  _convListSuccessNative,
);
final _convListErrorCallable =
    NativeCallable<Void Function(Pointer<Void>, Int, Pointer<Char>)>.listener(
  _convListErrorNative,
);

final _friendListSuccessCallable = NativeCallable<
    Void Function(Pointer<Void>, Pointer<AnyChatFriendList_C>)>.listener(
  _friendListSuccessNative,
);
final _friendListErrorCallable =
    NativeCallable<Void Function(Pointer<Void>, Int, Pointer<Char>)>.listener(
  _friendListErrorNative,
);

class AnyChatClient {
  late final AnyChatNativeBindings _bindings;
  late final AnyChatClientHandle _clientHandle;

  AnyChatAuthHandle? _authHandle;
  AnyChatMessageHandle? _messageHandle;
  AnyChatConvHandle? _convHandle;
  AnyChatFriendHandle? _friendHandle;

  final _connectionStateController =
      StreamController<ConnectionState>.broadcast();
  Stream<ConnectionState> get connectionStateStream =>
      _connectionStateController.stream;

  // Currently kept for API compatibility.
  final _messageReceivedController = StreamController<Message>.broadcast();
  Stream<Message> get messageReceivedStream =>
      _messageReceivedController.stream;

  // Currently kept for API compatibility.
  final _conversationUpdatedController =
      StreamController<Conversation>.broadcast();
  Stream<Conversation> get conversationUpdatedStream =>
      _conversationUpdatedController.stream;

  // Currently kept for API compatibility.
  final _friendRequestController = StreamController<FriendRequest>.broadcast();
  Stream<FriendRequest> get friendRequestStream =>
      _friendRequestController.stream;

  // Currently kept for API compatibility.
  final _incomingCallController = StreamController<CallSession>.broadcast();
  Stream<CallSession> get incomingCallStream => _incomingCallController.stream;

  late final NativeCallable<Void Function(Pointer<Void>, Int)>
      _connectionStateCallableNative;
  int? _connectionCallbackId;

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
    _globalBindings = _bindings;

    final config = calloc<AnyChatClientConfig_C>();
    final gatewayPtr = gatewayUrl.toNativeUtf8();
    final apiBasePtr = apiBaseUrl.toNativeUtf8();
    final devicePtr = deviceId.toNativeUtf8();
    final dbPathPtr = dbPath?.toNativeUtf8();

    config.ref.gateway_url = gatewayPtr.cast();
    config.ref.api_base_url = apiBasePtr.cast();
    config.ref.device_id = devicePtr.cast();
    config.ref.db_path = dbPathPtr != null ? dbPathPtr.cast() : nullptr;
    config.ref.connect_timeout_ms = connectTimeoutMs;
    config.ref.max_reconnect_attempts = maxReconnectAttempts;
    config.ref.auto_reconnect = autoReconnect ? 1 : 0;

    _clientHandle = _bindings.anychat_client_create(config);

    calloc.free(gatewayPtr);
    calloc.free(apiBasePtr);
    calloc.free(devicePtr);
    if (dbPathPtr != null) {
      calloc.free(dbPathPtr);
    }
    calloc.free(config);

    if (_clientHandle == nullptr) {
      throw Exception('Failed to create client');
    }

    _setupConnectionStateCallback();
  }

  AnyChatAuthHandle get _auth {
    _authHandle ??= _bindings.anychat_client_get_auth(_clientHandle);
    return _authHandle!;
  }

  AnyChatMessageHandle get _message {
    _messageHandle ??= _bindings.anychat_client_get_message(_clientHandle);
    return _messageHandle!;
  }

  AnyChatConvHandle get _conv {
    _convHandle ??= _bindings.anychat_client_get_conversation(_clientHandle);
    return _convHandle!;
  }

  AnyChatFriendHandle get _friend {
    _friendHandle ??= _bindings.anychat_client_get_friend(_clientHandle);
    return _friendHandle!;
  }

  void _setupConnectionStateCallback() {
    _connectionCallbackId = _registerCallback(this);
    _connectionStateCallableNative =
        NativeCallable<Void Function(Pointer<Void>, Int)>.listener(
      _connectionStateCallbackNative,
    );
    _bindings.anychat_client_set_connection_callback(
      _clientHandle,
      Pointer<Void>.fromAddress(_connectionCallbackId!),
      _connectionStateCallableNative.nativeFunction,
    );
  }

  ConnectionState get connectionState {
    final state = _bindings.anychat_client_get_connection_state(_clientHandle);
    return ConnectionState.fromInt(state);
  }

  void dispose() {
    _bindings.anychat_client_set_connection_callback(
        _clientHandle, nullptr, nullptr);
    _bindings.anychat_client_destroy(_clientHandle);
    _connectionStateCallableNative.close();

    if (_connectionCallbackId != null) {
      _unregisterCallback(_connectionCallbackId!);
      _connectionCallbackId = null;
    }

    _connectionStateController.close();
    _messageReceivedController.close();
    _conversationUpdatedController.close();
    _friendRequestController.close();
    _incomingCallController.close();
  }

  Future<AuthToken> login({
    required String account,
    required String password,
    int deviceType = 3,
    String clientVersion = '',
  }) {
    final completer = Completer<AuthToken>();
    final callbackId = _registerCallback(completer);

    final callback = calloc<AnyChatAuthTokenCallback_C>();
    callback.ref.struct_size = sizeOf<AnyChatAuthTokenCallback_C>();
    callback.ref.userdata = Pointer<Void>.fromAddress(callbackId);
    callback.ref.on_success = _authTokenSuccessCallable.nativeFunction;
    callback.ref.on_error = _authTokenErrorCallable.nativeFunction;

    final accountPtr = account.toNativeUtf8();
    final passwordPtr = password.toNativeUtf8();
    final clientVersionPtr = clientVersion.isEmpty
        ? nullptr
        : clientVersion.toNativeUtf8().cast<Char>();

    final ret = _bindings.anychat_client_login(
      _clientHandle,
      accountPtr.cast(),
      passwordPtr.cast(),
      deviceType,
      clientVersionPtr,
      callback,
    );

    calloc.free(accountPtr);
    calloc.free(passwordPtr);
    if (clientVersionPtr != nullptr) {
      calloc.free(clientVersionPtr);
    }
    calloc.free(callback);

    if (ret != ANYCHAT_OK) {
      _unregisterCallback(callbackId);
      completer.completeError(Exception(_dispatchErrorMessage(ret)));
    }

    return completer.future;
  }

  Future<AuthToken> register({
    required String phoneOrEmail,
    required String password,
    required String verifyCode,
    int deviceType = 3,
    String nickname = '',
    String clientVersion = '',
  }) {
    final completer = Completer<AuthToken>();
    final callbackId = _registerCallback(completer);

    final callback = calloc<AnyChatAuthTokenCallback_C>();
    callback.ref.struct_size = sizeOf<AnyChatAuthTokenCallback_C>();
    callback.ref.userdata = Pointer<Void>.fromAddress(callbackId);
    callback.ref.on_success = _authTokenSuccessCallable.nativeFunction;
    callback.ref.on_error = _authTokenErrorCallable.nativeFunction;

    final phonePtr = phoneOrEmail.toNativeUtf8();
    final passwordPtr = password.toNativeUtf8();
    final verifyCodePtr = verifyCode.toNativeUtf8();
    final nicknamePtr =
        nickname.isEmpty ? nullptr : nickname.toNativeUtf8().cast<Char>();
    final clientVersionPtr = clientVersion.isEmpty
        ? nullptr
        : clientVersion.toNativeUtf8().cast<Char>();

    final ret = _bindings.anychat_auth_register(
      _auth,
      phonePtr.cast(),
      passwordPtr.cast(),
      verifyCodePtr.cast(),
      deviceType,
      nicknamePtr,
      clientVersionPtr,
      callback,
    );

    calloc.free(phonePtr);
    calloc.free(passwordPtr);
    calloc.free(verifyCodePtr);
    if (nicknamePtr != nullptr) {
      calloc.free(nicknamePtr);
    }
    if (clientVersionPtr != nullptr) {
      calloc.free(clientVersionPtr);
    }
    calloc.free(callback);

    if (ret != ANYCHAT_OK) {
      _unregisterCallback(callbackId);
      completer.completeError(Exception(_dispatchErrorMessage(ret)));
    }

    return completer.future;
  }

  bool get isLoggedIn {
    return _bindings.anychat_client_is_logged_in(_clientHandle) != 0;
  }

  AuthToken? get currentToken {
    if (!isLoggedIn) {
      return null;
    }

    final token = calloc<AnyChatAuthToken_C>();
    final ret =
        _bindings.anychat_client_get_current_token(_clientHandle, token);
    if (ret != ANYCHAT_OK) {
      calloc.free(token);
      return null;
    }

    final value = AuthToken(
      accessToken: _copyFixedStringStatic(token.ref.access_token, 512),
      refreshToken: _copyFixedStringStatic(token.ref.refresh_token, 512),
      expiresAtMs: token.ref.expires_at_ms,
    );
    calloc.free(token);
    return value;
  }

  Future<void> logout() {
    final completer = Completer<void>();
    final callbackId = _registerCallback(completer);

    final callback = calloc<AnyChatAuthResultCallback_C>();
    callback.ref.struct_size = sizeOf<AnyChatAuthResultCallback_C>();
    callback.ref.userdata = Pointer<Void>.fromAddress(callbackId);
    callback.ref.on_success = _voidSuccessCallable.nativeFunction;
    callback.ref.on_error = _voidErrorCallable.nativeFunction;

    final ret = _bindings.anychat_client_logout(_clientHandle, callback);
    calloc.free(callback);

    if (ret != ANYCHAT_OK) {
      _unregisterCallback(callbackId);
      completer.completeError(Exception(_dispatchErrorMessage(ret)));
    }

    return completer.future;
  }

  Future<void> sendTextMessage({
    required String sessionId,
    required String content,
  }) {
    final completer = Completer<void>();
    final callbackId = _registerCallback(completer);

    final callback = calloc<AnyChatMessageCallback_C>();
    callback.ref.struct_size = sizeOf<AnyChatMessageCallback_C>();
    callback.ref.userdata = Pointer<Void>.fromAddress(callbackId);
    callback.ref.on_success = _voidSuccessCallable.nativeFunction;
    callback.ref.on_error = _voidErrorCallable.nativeFunction;

    final sessionPtr = sessionId.toNativeUtf8();
    final contentPtr = content.toNativeUtf8();

    final ret = _bindings.anychat_message_send_text(
      _message,
      sessionPtr.cast(),
      contentPtr.cast(),
      callback,
    );

    calloc.free(sessionPtr);
    calloc.free(contentPtr);
    calloc.free(callback);

    if (ret != ANYCHAT_OK) {
      _unregisterCallback(callbackId);
      completer.completeError(Exception(_dispatchErrorMessage(ret)));
    }

    return completer.future;
  }

  Future<List<Message>> getMessageHistory({
    required String sessionId,
    int beforeTimestampMs = 0,
    int limit = 20,
  }) {
    final completer = Completer<List<Message>>();
    final callbackId = _registerCallback(completer);

    final callback = calloc<AnyChatMessageListCallback_C>();
    callback.ref.struct_size = sizeOf<AnyChatMessageListCallback_C>();
    callback.ref.userdata = Pointer<Void>.fromAddress(callbackId);
    callback.ref.on_success = _messageListSuccessCallable.nativeFunction;
    callback.ref.on_error = _messageListErrorCallable.nativeFunction;

    final sessionPtr = sessionId.toNativeUtf8();
    final ret = _bindings.anychat_message_get_history(
      _message,
      sessionPtr.cast(),
      beforeTimestampMs,
      limit,
      callback,
    );

    calloc.free(sessionPtr);
    calloc.free(callback);

    if (ret != ANYCHAT_OK) {
      _unregisterCallback(callbackId);
      completer.completeError(Exception(_dispatchErrorMessage(ret)));
    }

    return completer.future;
  }

  Future<List<Conversation>> getConversations() {
    final completer = Completer<List<Conversation>>();
    final callbackId = _registerCallback(completer);

    final callback = calloc<AnyChatConvListCallback_C>();
    callback.ref.struct_size = sizeOf<AnyChatConvListCallback_C>();
    callback.ref.userdata = Pointer<Void>.fromAddress(callbackId);
    callback.ref.on_success = _convListSuccessCallable.nativeFunction;
    callback.ref.on_error = _convListErrorCallable.nativeFunction;

    final ret = _bindings.anychat_conv_get_list(_conv, callback);
    calloc.free(callback);

    if (ret != ANYCHAT_OK) {
      _unregisterCallback(callbackId);
      completer.completeError(Exception(_dispatchErrorMessage(ret)));
    }

    return completer.future;
  }

  Future<List<Friend>> getFriends() {
    final completer = Completer<List<Friend>>();
    final callbackId = _registerCallback(completer);

    final callback = calloc<AnyChatFriendListCallback_C>();
    callback.ref.struct_size = sizeOf<AnyChatFriendListCallback_C>();
    callback.ref.userdata = Pointer<Void>.fromAddress(callbackId);
    callback.ref.on_success = _friendListSuccessCallable.nativeFunction;
    callback.ref.on_error = _friendListErrorCallable.nativeFunction;

    final ret = _bindings.anychat_friend_get_list(_friend, callback);
    calloc.free(callback);

    if (ret != ANYCHAT_OK) {
      _unregisterCallback(callbackId);
      completer.completeError(Exception(_dispatchErrorMessage(ret)));
    }

    return completer.future;
  }
}
