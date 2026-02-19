/**
 * app.ts - Example application demonstrating AnyChat Web SDK usage
 */

import { createAnyChatClient, AnyChatClient, ConnectionState, Message, Conversation } from '../dist/index.js';

class ChatApp {
  private client: AnyChatClient | null = null;
  private currentConversation: string | null = null;
  private isLoggedIn: boolean = false;

  async init() {
    try {
      // Initialize the AnyChat client
      this.client = await createAnyChatClient({
        gatewayUrl: 'wss://api.anychat.io',
        apiBaseUrl: 'https://api.anychat.io/api/v1',
        deviceId: this.getOrCreateDeviceId(),
        dbPath: ':memory:', // Use in-memory database for web
      }, '../lib/anychat.js');

      this.setupEventListeners();
      this.renderLoginForm();
    } catch (error) {
      console.error('Failed to initialize client:', error);
      this.showError('Failed to initialize AnyChat SDK. Please refresh the page.');
    }
  }

  private getOrCreateDeviceId(): string {
    let deviceId = localStorage.getItem('anychat_device_id');
    if (!deviceId) {
      deviceId = 'web-' + Math.random().toString(36).substring(2, 15);
      localStorage.setItem('anychat_device_id', deviceId);
    }
    return deviceId;
  }

  private setupEventListeners() {
    if (!this.client) return;

    // Connection state changes
    this.client.on('connectionStateChanged', (state) => {
      console.log('Connection state:', state);
      this.updateConnectionStatus(state);
    });

    // Incoming messages
    this.client.on('messageReceived', (message) => {
      console.log('New message:', message);
      if (message.convId === this.currentConversation) {
        this.appendMessage(message);
      }
      this.updateConversationList();
    });

    // Conversation updates
    this.client.on('conversationUpdated', (conversation) => {
      console.log('Conversation updated:', conversation);
      this.updateConversationList();
    });

    // Friend requests
    this.client.on('friendRequest', (request) => {
      console.log('Friend request:', request);
      this.showNotification(`Friend request from ${request.fromUserInfo.username}`);
    });

    // Auth expired
    this.client.on('authExpired', () => {
      console.log('Auth expired');
      this.logout();
    });
  }

  private updateConnectionStatus(state: ConnectionState) {
    const statusIndicator = document.querySelector('.status-indicator');
    const statusText = document.querySelector('.status-text');

    if (!statusIndicator || !statusText) return;

    const stateNames = {
      [ConnectionState.Disconnected]: 'Disconnected',
      [ConnectionState.Connecting]: 'Connecting...',
      [ConnectionState.Connected]: 'Connected',
      [ConnectionState.Reconnecting]: 'Reconnecting...',
    };

    statusText.textContent = stateNames[state] || 'Unknown';

    if (state === ConnectionState.Connected) {
      statusIndicator.classList.add('connected');
    } else {
      statusIndicator.classList.remove('connected');
    }
  }

  private renderLoginForm() {
    const app = document.getElementById('app');
    if (!app) return;

    app.innerHTML = `
      <div class="container">
        <div class="login-form">
          <h2>Login to AnyChat</h2>
          <form id="loginForm">
            <div class="form-group">
              <label for="account">Email or Phone</label>
              <input type="text" id="account" name="account" required placeholder="user@example.com">
            </div>
            <div class="form-group">
              <label for="password">Password</label>
              <input type="password" id="password" name="password" required placeholder="••••••••">
            </div>
            <div class="error" id="loginError"></div>
            <div class="form-actions">
              <button type="submit">Login</button>
            </div>
          </form>
        </div>
      </div>
    `;

    const form = document.getElementById('loginForm') as HTMLFormElement;
    form.addEventListener('submit', (e) => this.handleLogin(e));
  }

  private async handleLogin(e: Event) {
    e.preventDefault();
    if (!this.client) return;

    const form = e.target as HTMLFormElement;
    const formData = new FormData(form);
    const account = formData.get('account') as string;
    const password = formData.get('password') as string;
    const errorEl = document.getElementById('loginError');

    if (errorEl) errorEl.textContent = '';

    try {
      // Connect to server
      this.client.connect();

      // Login
      const token = await this.client.login(account, password, 'web');
      console.log('Login successful:', token);

      // Store token
      localStorage.setItem('anychat_token', JSON.stringify(token));

      this.isLoggedIn = true;
      this.renderChatInterface();
      this.loadConversations();
    } catch (error: any) {
      console.error('Login failed:', error);
      if (errorEl) {
        errorEl.textContent = error.message || 'Login failed. Please try again.';
      }
    }
  }

  private renderChatInterface() {
    const app = document.getElementById('app');
    if (!app) return;

    app.innerHTML = `
      <div class="container">
        <div class="header">
          <h1>AnyChat</h1>
          <div class="status">
            <div class="status-indicator"></div>
            <span class="status-text">Connecting...</span>
          </div>
        </div>
        <div class="main">
          <div class="sidebar">
            <ul class="conversation-list" id="conversationList">
              <li class="loading">Loading conversations...</li>
            </ul>
          </div>
          <div class="chat-area">
            <div class="messages" id="messages">
              <div style="text-align: center; color: #999; padding: 40px;">
                Select a conversation to start chatting
              </div>
            </div>
            <div class="message-input">
              <input type="text" id="messageInput" placeholder="Type a message..." disabled>
              <button id="sendButton" disabled>Send</button>
            </div>
          </div>
        </div>
      </div>
    `;

    const sendButton = document.getElementById('sendButton');
    const messageInput = document.getElementById('messageInput') as HTMLInputElement;

    sendButton?.addEventListener('click', () => this.sendMessage());
    messageInput?.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') {
        this.sendMessage();
      }
    });
  }

  private async loadConversations() {
    if (!this.client) return;

    try {
      const conversations = await this.client.getConversationList();
      this.renderConversations(conversations);
    } catch (error) {
      console.error('Failed to load conversations:', error);
    }
  }

  private renderConversations(conversations: Conversation[]) {
    const list = document.getElementById('conversationList');
    if (!list) return;

    if (conversations.length === 0) {
      list.innerHTML = '<li style="padding: 20px; text-align: center; color: #999;">No conversations yet</li>';
      return;
    }

    list.innerHTML = conversations.map((conv) => `
      <li class="conversation-item" data-conv-id="${conv.convId}">
        <div style="font-weight: 500; margin-bottom: 4px;">${conv.targetId}</div>
        <div style="font-size: 14px; color: #666; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;">
          ${conv.lastMsgText || 'No messages'}
        </div>
        ${conv.unreadCount > 0 ? `<div style="display: inline-block; background: #ff5252; color: white; padding: 2px 8px; border-radius: 10px; font-size: 12px; margin-top: 4px;">${conv.unreadCount}</div>` : ''}
      </li>
    `).join('');

    // Add click handlers
    list.querySelectorAll('.conversation-item').forEach((item) => {
      item.addEventListener('click', () => {
        const convId = item.getAttribute('data-conv-id');
        if (convId) {
          this.openConversation(convId);
        }
      });
    });
  }

  private async openConversation(convId: string) {
    if (!this.client) return;

    this.currentConversation = convId;

    // Update active state
    document.querySelectorAll('.conversation-item').forEach((item) => {
      item.classList.remove('active');
      if (item.getAttribute('data-conv-id') === convId) {
        item.classList.add('active');
      }
    });

    // Enable message input
    const messageInput = document.getElementById('messageInput') as HTMLInputElement;
    const sendButton = document.getElementById('sendButton') as HTMLButtonElement;
    if (messageInput) messageInput.disabled = false;
    if (sendButton) sendButton.disabled = false;

    // Load message history
    try {
      const messages = await this.client.getMessageHistory(convId, 0, 50);
      this.renderMessages(messages);

      // Mark as read
      if (messages.length > 0) {
        await this.client.markConversationRead(convId);
      }
    } catch (error) {
      console.error('Failed to load messages:', error);
    }
  }

  private renderMessages(messages: Message[]) {
    const messagesContainer = document.getElementById('messages');
    if (!messagesContainer) return;

    messagesContainer.innerHTML = messages.map((msg) => {
      const isSent = msg.senderId === 'me'; // This should be checked against current user ID
      return `
        <div class="message ${isSent ? 'sent' : 'received'}">
          ${msg.content}
        </div>
      `;
    }).join('');

    // Scroll to bottom
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
  }

  private appendMessage(message: Message) {
    const messagesContainer = document.getElementById('messages');
    if (!messagesContainer) return;

    const isSent = message.senderId === 'me'; // This should be checked against current user ID
    const messageEl = document.createElement('div');
    messageEl.className = `message ${isSent ? 'sent' : 'received'}`;
    messageEl.textContent = message.content;

    messagesContainer.appendChild(messageEl);
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
  }

  private async sendMessage() {
    if (!this.client || !this.currentConversation) return;

    const input = document.getElementById('messageInput') as HTMLInputElement;
    const content = input.value.trim();

    if (!content) return;

    try {
      await this.client.sendTextMessage(this.currentConversation, content);
      input.value = '';

      // Add message to UI optimistically
      this.appendMessage({
        messageId: '',
        localId: '',
        convId: this.currentConversation,
        senderId: 'me',
        contentType: 'text',
        type: 0,
        content,
        seq: 0,
        replyTo: '',
        timestamp: Date.now(),
        status: 0,
        sendState: 0,
        isRead: false,
      });
    } catch (error) {
      console.error('Failed to send message:', error);
      this.showError('Failed to send message');
    }
  }

  private updateConversationList() {
    this.loadConversations();
  }

  private logout() {
    localStorage.removeItem('anychat_token');
    this.isLoggedIn = false;
    if (this.client) {
      this.client.disconnect();
    }
    this.renderLoginForm();
  }

  private showError(message: string) {
    alert(message);
  }

  private showNotification(message: string) {
    console.log('Notification:', message);
  }
}

// Initialize the app
const app = new ChatApp();
app.init();
