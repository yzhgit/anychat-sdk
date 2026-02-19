export interface ClientConfig {
  gatewayUrl: string;
  apiBaseUrl: string;
  connectTimeoutMs?: number;
  autoReconnect?: boolean;
}

export type MessageType = 'text' | 'image' | 'file' | 'audio' | 'video';

export interface Message {
  messageId: string;
  sessionId: string;
  senderId: string;
  type: MessageType;
  content: string;
  timestamp: number;  // unix ms
  isRead: boolean;
}

export interface AuthToken {
  accessToken: string;
  refreshToken: string;
  expiresAt: number;  // unix seconds
}
