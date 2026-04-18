/**
 * types.ts - TypeScript type definitions for AnyChat SDK
 */

// Connection States
export enum ConnectionState {
  Disconnected = 0,
  Connecting = 1,
  Connected = 2,
  Reconnecting = 3,
}

// Message Types
export enum MessageType {
  Text = 0,
  Image = 1,
  File = 2,
  Audio = 3,
  Video = 4,
}

// Conversation Types
export enum ConversationType {
  Private = 0,
  Group = 1,
}

// Message Send States
export enum MessageSendState {
  Pending = 0,
  Sent = 1,
  Failed = 2,
}

// Group Roles
export enum GroupRole {
  Owner = 1,
  Admin = 2,
  Member = 3,
}

// Auth-related enums
export enum DeviceType {
  Unspecified = 0,
  IOS = 1,
  Android = 2,
  Web = 3,
  PC = 4,
  H5 = 5,
}

export enum VerificationTargetType {
  Unspecified = 0,
  SMS = 1,
  Email = 2,
}

export enum VerificationPurpose {
  Unspecified = 0,
  Register = 1,
  Login = 2,
  ResetPassword = 3,
  BindPhone = 4,
  ChangePhone = 5,
  BindEmail = 6,
  ChangeEmail = 7,
}

export enum FriendSource {
  Unspecified = 0,
  Search = 1,
  QRCode = 2,
  Group = 3,
  Contacts = 4,
}

export enum FriendRequestStatus {
  Unspecified = 0,
  Pending = 1,
  Accepted = 2,
  Rejected = 3,
  Expired = 4,
}

export enum FriendRequestAction {
  Unspecified = 0,
  Accept = 1,
  Reject = 2,
}

export enum FriendRequestQueryType {
  Unspecified = 0,
  Received = 1,
  Sent = 2,
}

// Client Configuration
export interface ClientConfig {
  gatewayUrl: string;
  apiBaseUrl: string;
  deviceId: string;
  dbPath?: string;
  connectTimeoutMs?: number;
  maxReconnectAttempts?: number;
  autoReconnect?: boolean;
}

// Auth Token
export interface AuthToken {
  accessToken: string;
  refreshToken: string;
  expiresAt: number;
}

// User Info
export interface UserInfo {
  userId: string;
  username: string;
  avatarUrl: string;
}

// Message
export interface Message {
  messageId: string;
  localId: string;
  convId: string;
  senderId: string;
  contentType: number;
  type: MessageType;
  content: string;
  seq: number;
  replyTo: string;
  timestamp: number;
  status: number;
  sendState: MessageSendState;
  isRead: boolean;
}

// Conversation
export interface Conversation {
  convId: string;
  convType: ConversationType;
  targetId: string;
  lastMsgId: string;
  lastMsgText: string;
  lastMsgTime: number;
  unreadCount: number;
  isPinned: boolean;
  isMuted: boolean;
  updatedAt: number;
}

// Friend
export interface Friend {
  userId: string;
  remark: string;
  updatedAt: number;
  isDeleted: boolean;
  userInfo: UserInfo;
}

// Friend Request
export interface FriendRequest {
  requestId: number;
  fromUserId: string;
  toUserId: string;
  message: string;
  source: FriendSource;
  status: FriendRequestStatus;
  createdAt: number;
  fromUserInfo: UserInfo;
}

// Group
export interface Group {
  groupId: string;
  name: string;
  avatarUrl: string;
  ownerId: string;
  memberCount: number;
  myRole: GroupRole;
  joinVerify: boolean;
  updatedAt: number;
}

// Group Member
export interface GroupMember {
  userId: string;
  groupNickname: string;
  role: GroupRole;
  isMuted: boolean;
  joinedAt: number;
  userInfo: UserInfo;
}

// Event Types
export type ConnectionStateListener = (state: ConnectionState) => void;
export type MessageReceivedListener = (message: Message) => void;
export type ConversationUpdatedListener = (conversation: Conversation) => void;
export type FriendRequestListener = (request: FriendRequest) => void;
export type FriendListChangedListener = () => void;
export type GroupInvitedListener = (group: Group, inviterId: string) => void;
export type GroupUpdatedListener = (group: Group) => void;
export type AuthExpiredListener = () => void;

// Error class
export class AnyChatError extends Error {
  constructor(message: string) {
    super(message);
    this.name = 'AnyChatError';
  }
}
