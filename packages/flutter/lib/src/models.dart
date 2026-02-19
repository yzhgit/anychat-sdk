// Dart data models that mirror the C structs

/// Connection states
enum ConnectionState {
  disconnected,
  connecting,
  connected,
  reconnecting;

  static ConnectionState fromInt(int value) {
    if (value < 0 || value >= ConnectionState.values.length) {
      return ConnectionState.disconnected;
    }
    return ConnectionState.values[value];
  }
}

/// Message types
enum MessageType {
  text,
  image,
  file,
  audio,
  video;

  static MessageType fromInt(int value) {
    if (value < 0 || value >= MessageType.values.length) {
      return MessageType.text;
    }
    return MessageType.values[value];
  }
}

/// Conversation types
enum ConversationType {
  private_,
  group;

  static ConversationType fromInt(int value) {
    return value == 0 ? ConversationType.private_ : ConversationType.group;
  }
}

/// Send states
enum SendState {
  pending,
  sent,
  failed;

  static SendState fromInt(int value) {
    if (value < 0 || value >= SendState.values.length) {
      return SendState.pending;
    }
    return SendState.values[value];
  }
}

/// Call types
enum CallType {
  audio,
  video;

  static CallType fromInt(int value) {
    return value == 0 ? CallType.audio : CallType.video;
  }
}

/// Call status
enum CallStatus {
  ringing,
  connected,
  ended,
  rejected,
  missed,
  cancelled;

  static CallStatus fromInt(int value) {
    if (value < 0 || value >= CallStatus.values.length) {
      return CallStatus.ringing;
    }
    return CallStatus.values[value];
  }
}

/// Group roles
enum GroupRole {
  owner,
  admin,
  member;

  static GroupRole fromInt(int value) {
    if (value < 0 || value >= GroupRole.values.length) {
      return GroupRole.member;
    }
    return GroupRole.values[value];
  }
}

/// User info
class UserInfo {
  final String userId;
  final String username;
  final String avatarUrl;

  UserInfo({
    required this.userId,
    required this.username,
    required this.avatarUrl,
  });

  @override
  String toString() =>
      'UserInfo(userId: $userId, username: $username, avatarUrl: $avatarUrl)';
}

/// Auth token
class AuthToken {
  final String accessToken;
  final String refreshToken;
  final int expiresAtMs;

  AuthToken({
    required this.accessToken,
    required this.refreshToken,
    required this.expiresAtMs,
  });

  bool get isExpired => DateTime.now().millisecondsSinceEpoch > expiresAtMs;

  @override
  String toString() =>
      'AuthToken(accessToken: ${accessToken.substring(0, 20)}..., expiresAtMs: $expiresAtMs)';
}

/// Message
class Message {
  final String messageId;
  final String localId;
  final String convId;
  final String senderId;
  final String contentType;
  final MessageType type;
  final String content;
  final int seq;
  final String replyTo;
  final int timestampMs;
  final int status;
  final SendState sendState;
  final bool isRead;

  Message({
    required this.messageId,
    required this.localId,
    required this.convId,
    required this.senderId,
    required this.contentType,
    required this.type,
    required this.content,
    required this.seq,
    required this.replyTo,
    required this.timestampMs,
    required this.status,
    required this.sendState,
    required this.isRead,
  });

  DateTime get timestamp =>
      DateTime.fromMillisecondsSinceEpoch(timestampMs);

  @override
  String toString() =>
      'Message(messageId: $messageId, senderId: $senderId, content: $content)';
}

/// Conversation
class Conversation {
  final String convId;
  final ConversationType convType;
  final String targetId;
  final String lastMsgId;
  final String lastMsgText;
  final int lastMsgTimeMs;
  final int unreadCount;
  final bool isPinned;
  final bool isMuted;
  final int updatedAtMs;

  Conversation({
    required this.convId,
    required this.convType,
    required this.targetId,
    required this.lastMsgId,
    required this.lastMsgText,
    required this.lastMsgTimeMs,
    required this.unreadCount,
    required this.isPinned,
    required this.isMuted,
    required this.updatedAtMs,
  });

  DateTime get lastMsgTime =>
      DateTime.fromMillisecondsSinceEpoch(lastMsgTimeMs);
  DateTime get updatedAt =>
      DateTime.fromMillisecondsSinceEpoch(updatedAtMs);

  @override
  String toString() =>
      'Conversation(convId: $convId, unreadCount: $unreadCount, lastMsgText: $lastMsgText)';
}

/// Friend
class Friend {
  final String userId;
  final String remark;
  final int updatedAtMs;
  final bool isDeleted;
  final UserInfo userInfo;

  Friend({
    required this.userId,
    required this.remark,
    required this.updatedAtMs,
    required this.isDeleted,
    required this.userInfo,
  });

  @override
  String toString() =>
      'Friend(userId: $userId, remark: $remark, userInfo: $userInfo)';
}

/// Friend request
class FriendRequest {
  final int requestId;
  final String fromUserId;
  final String toUserId;
  final String message;
  final String status;
  final int createdAtMs;
  final UserInfo fromUserInfo;

  FriendRequest({
    required this.requestId,
    required this.fromUserId,
    required this.toUserId,
    required this.message,
    required this.status,
    required this.createdAtMs,
    required this.fromUserInfo,
  });

  DateTime get createdAt =>
      DateTime.fromMillisecondsSinceEpoch(createdAtMs);

  @override
  String toString() =>
      'FriendRequest(requestId: $requestId, from: $fromUserId, status: $status)';
}

/// Group
class Group {
  final String groupId;
  final String name;
  final String avatarUrl;
  final String ownerId;
  final int memberCount;
  final GroupRole myRole;
  final bool joinVerify;
  final int updatedAtMs;

  Group({
    required this.groupId,
    required this.name,
    required this.avatarUrl,
    required this.ownerId,
    required this.memberCount,
    required this.myRole,
    required this.joinVerify,
    required this.updatedAtMs,
  });

  @override
  String toString() =>
      'Group(groupId: $groupId, name: $name, memberCount: $memberCount)';
}

/// Group member
class GroupMember {
  final String userId;
  final String groupNickname;
  final GroupRole role;
  final bool isMuted;
  final int joinedAtMs;
  final UserInfo userInfo;

  GroupMember({
    required this.userId,
    required this.groupNickname,
    required this.role,
    required this.isMuted,
    required this.joinedAtMs,
    required this.userInfo,
  });

  @override
  String toString() =>
      'GroupMember(userId: $userId, nickname: $groupNickname, role: $role)';
}

/// File info
class FileInfo {
  final String fileId;
  final String fileName;
  final String fileType;
  final int fileSizeBytes;
  final String mimeType;
  final String downloadUrl;
  final int createdAtMs;

  FileInfo({
    required this.fileId,
    required this.fileName,
    required this.fileType,
    required this.fileSizeBytes,
    required this.mimeType,
    required this.downloadUrl,
    required this.createdAtMs,
  });

  @override
  String toString() =>
      'FileInfo(fileId: $fileId, fileName: $fileName, size: $fileSizeBytes)';
}

/// User profile
class UserProfile {
  final String userId;
  final String nickname;
  final String avatarUrl;
  final String phone;
  final String email;
  final String signature;
  final String region;
  final int gender;
  final int createdAtMs;

  UserProfile({
    required this.userId,
    required this.nickname,
    required this.avatarUrl,
    required this.phone,
    required this.email,
    required this.signature,
    required this.region,
    required this.gender,
    required this.createdAtMs,
  });

  @override
  String toString() =>
      'UserProfile(userId: $userId, nickname: $nickname, email: $email)';
}

/// User settings
class UserSettings {
  final bool notificationEnabled;
  final bool soundEnabled;
  final bool vibrationEnabled;
  final bool messagePreviewEnabled;
  final bool friendVerifyRequired;
  final bool searchByPhone;
  final bool searchById;
  final String language;

  UserSettings({
    required this.notificationEnabled,
    required this.soundEnabled,
    required this.vibrationEnabled,
    required this.messagePreviewEnabled,
    required this.friendVerifyRequired,
    required this.searchByPhone,
    required this.searchById,
    required this.language,
  });

  @override
  String toString() => 'UserSettings(language: $language, notifications: $notificationEnabled)';
}

/// Call session
class CallSession {
  final String callId;
  final String callerId;
  final String calleeId;
  final CallType callType;
  final CallStatus status;
  final String roomName;
  final String token;
  final int startedAt;
  final int connectedAt;
  final int endedAt;
  final int duration;

  CallSession({
    required this.callId,
    required this.callerId,
    required this.calleeId,
    required this.callType,
    required this.status,
    required this.roomName,
    required this.token,
    required this.startedAt,
    required this.connectedAt,
    required this.endedAt,
    required this.duration,
  });

  @override
  String toString() =>
      'CallSession(callId: $callId, status: $status, duration: ${duration}s)';
}

/// Meeting room
class MeetingRoom {
  final String roomId;
  final String creatorId;
  final String title;
  final String roomName;
  final String token;
  final bool hasPassword;
  final int maxParticipants;
  final bool isActive;
  final int startedAt;
  final int createdAtMs;

  MeetingRoom({
    required this.roomId,
    required this.creatorId,
    required this.title,
    required this.roomName,
    required this.token,
    required this.hasPassword,
    required this.maxParticipants,
    required this.isActive,
    required this.startedAt,
    required this.createdAtMs,
  });

  @override
  String toString() =>
      'MeetingRoom(roomId: $roomId, title: $title, maxParticipants: $maxParticipants)';
}
