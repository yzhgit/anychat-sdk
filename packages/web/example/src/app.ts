/**
 * app.ts - Example application demonstrating @anychat/sdk usage
 */

import { createAnyChatClient, AnyChatClient, ConnectionState, Message, Conversation } from '@anychat/sdk';

class ChatApp {
  private client: AnyChatClient | null = null;
  private currentConversation: string | null = null;
  private currentUserId: string | null = null;

  async init() {
    try {
      // Initialize the AnyChat client
      // Local development server (change to your machine's IP)
      this.client = await createAnyChatClient({
        gatewayUrl: 'ws://192.168.2.100:8080',
        apiBaseUrl: 'http://192.168.2.100:8080/api/v1',
        deviceId: this.getOrCreateDeviceId(),
        dbPath: ':memory:', // Use in-memory database for web
        connectTimeoutMs: 30000,
        maxReconnectAttempts: 5,
        autoReconnect: true,
      }, '/lib/anychat.js');

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
      console.log('Connection state:', ConnectionState[state]);
      this.updateConnectionStatus(state);
    });

    // Incoming messages
    this.client.on('messageReceived', (message) => {
      console.log('New message:', message);
      if (message.convId === this.currentConversation) {
        this.appendMessage(message);
        this.client?.markMessageRead(message.convId, message.messageId);
      }
      this.loadConversations();
    });

    // Conversation updates
    this.client.on('conversationUpdated', (conversation) => {
      console.log('Conversation updated:', conversation);
      this.loadConversations();
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
          <h2>Welcome to AnyChat</h2>
          <div class="form-tabs">
            <button class="tab-button active" id="loginTab">Login</button>
            <button class="tab-button" id="registerTab">Register</button>
          </div>
          <form id="loginForm">
            <div class="form-group">
              <label for="account">Email or Phone</label>
              <input type="text" id="account" name="account" required placeholder="user@example.com">
            </div>
            <div class="form-group">
              <label for="password">Password</label>
              <input type="password" id="password" name="password" required placeholder="Enter your password">
            </div>
            <div class="form-group register-only" style="display: none;">
              <label for="nickname">Nickname</label>
              <input type="text" id="nickname" name="nickname" placeholder="Your display name">
            </div>
            <div class="form-group register-only" style="display: none;">
              <label for="verifyCode">Verification Code</label>
              <input type="text" id="verifyCode" name="verifyCode" placeholder="Enter verification code">
            </div>
            <div id="loginError"></div>
            <div class="form-actions">
              <button type="submit" id="submitButton">Sign In</button>
            </div>
          </form>
        </div>
      </div>
    `;

    const loginTab = document.getElementById('loginTab');
    const registerTab = document.getElementById('registerTab');
    const registerFields = document.querySelectorAll('.register-only');
    const submitButton = document.getElementById('submitButton');
    const form = document.getElementById('loginForm') as HTMLFormElement;

    let isRegisterMode = false;

    loginTab?.addEventListener('click', () => {
      isRegisterMode = false;
      loginTab.classList.add('active');
      registerTab?.classList.remove('active');
      registerFields.forEach(field => (field as HTMLElement).style.display = 'none');
      if (submitButton) submitButton.textContent = 'Sign In';
      const nicknameInput = document.getElementById('nickname') as HTMLInputElement;
      const verifyCodeInput = document.getElementById('verifyCode') as HTMLInputElement;
      if (nicknameInput) nicknameInput.required = false;
      if (verifyCodeInput) verifyCodeInput.required = false;
    });

    registerTab?.addEventListener('click', () => {
      isRegisterMode = true;
      registerTab.classList.add('active');
      loginTab?.classList.remove('active');
      registerFields.forEach(field => (field as HTMLElement).style.display = 'block');
      if (submitButton) submitButton.textContent = 'Register';
      const nicknameInput = document.getElementById('nickname') as HTMLInputElement;
      const verifyCodeInput = document.getElementById('verifyCode') as HTMLInputElement;
      if (nicknameInput) nicknameInput.required = true;
      if (verifyCodeInput) verifyCodeInput.required = true;
    });

    form.addEventListener('submit', (e) => {
      if (isRegisterMode) {
        this.handleRegister(e);
      } else {
        this.handleLogin(e);
      }
    });
  }

  private async handleLogin(e: Event) {
    e.preventDefault();
    if (!this.client) return;

    const form = e.target as HTMLFormElement;
    const formData = new FormData(form);
    const account = formData.get('account') as string;
    const password = formData.get('password') as string;
    const errorEl = document.getElementById('loginError');

    if (errorEl) errorEl.innerHTML = '';

    try {
      // Connect to server
      this.client.connect();

      // Login
      const token = await this.client.login(account, password, 'Web');
      console.log('Login successful:', token);

      // Store token
      localStorage.setItem('anychat_token', JSON.stringify(token));
      this.currentUserId = account; // In a real app, this should come from user info

      this.renderChatInterface();
      await this.loadConversations();
    } catch (error: any) {
      console.error('Login failed:', error);
      if (errorEl) {
        errorEl.innerHTML = `<div class="error">${error.message || 'Login failed. Please try again.'}</div>`;
      }
    }
  }

  private async handleRegister(e: Event) {
    e.preventDefault();
    if (!this.client) return;

    const form = e.target as HTMLFormElement;
    const formData = new FormData(form);
    const account = formData.get('account') as string;
    const password = formData.get('password') as string;
    const nickname = formData.get('nickname') as string;
    const verifyCode = formData.get('verifyCode') as string;
    const errorEl = document.getElementById('loginError');

    if (errorEl) errorEl.innerHTML = '';

    try {
      // Connect to server
      this.client.connect();

      // Register
      const token = await this.client.register(account, password, verifyCode, 'Web', nickname);
      console.log('Registration successful:', token);

      // Store token
      localStorage.setItem('anychat_token', JSON.stringify(token));
      this.currentUserId = account; // In a real app, this should come from user info

      this.renderChatInterface();
      await this.loadConversations();
    } catch (error: any) {
      console.error('Registration failed:', error);
      if (errorEl) {
        errorEl.innerHTML = `<div class="error">${error.message || 'Registration failed. Please try again.'}</div>`;
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
              <li class="empty-state">
                <div>Loading conversations...</div>
              </li>
            </ul>
          </div>
          <div class="chat-area">
            <div class="messages" id="messages">
              <div class="empty-state">
                <svg fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 12h.01M12 12h.01M16 12h.01M21 12c0 4.418-4.03 8-9 8a9.863 9.863 0 01-4.255-.949L3 20l1.395-3.72C3.512 15.042 3 13.574 3 12c0-4.418 4.03-8 9-8s9 3.582 9 8z"></path>
                </svg>
                <div>Select a conversation to start chatting</div>
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
      if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
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
      list.innerHTML = '<li class="empty-state"><div>No conversations yet</div></li>';
      return;
    }

    list.innerHTML = conversations.map((conv) => {
      const initial = conv.targetId.charAt(0).toUpperCase();
      return `
        <li class="conversation-item ${conv.convId === this.currentConversation ? 'active' : ''}" data-conv-id="${conv.convId}">
          <div class="conversation-avatar">${initial}</div>
          <div class="conversation-info">
            <div class="conversation-name">${conv.targetId}</div>
            <div class="conversation-last-msg">${conv.lastMsgText || 'No messages yet'}</div>
          </div>
          ${conv.unreadCount > 0 ? `<div class="unread-badge">${conv.unreadCount}</div>` : ''}
        </li>
      `;
    }).join('');

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

    if (messages.length === 0) {
      messagesContainer.innerHTML = '<div class="empty-state"><div>No messages yet. Start the conversation!</div></div>';
      return;
    }

    messagesContainer.innerHTML = messages.map((msg) => {
      const isSent = msg.senderId === this.currentUserId;
      const time = new Date(msg.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
      return `
        <div class="message ${isSent ? 'sent' : 'received'}">
          <div>${msg.content}</div>
          <div class="message-time">${time}</div>
        </div>
      `;
    }).join('');

    // Scroll to bottom
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
  }

  private appendMessage(message: Message) {
    const messagesContainer = document.getElementById('messages');
    if (!messagesContainer) return;

    // Remove empty state if present
    const emptyState = messagesContainer.querySelector('.empty-state');
    if (emptyState) {
      emptyState.remove();
    }

    const isSent = message.senderId === this.currentUserId;
    const time = new Date(message.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    const messageEl = document.createElement('div');
    messageEl.className = `message ${isSent ? 'sent' : 'received'}`;
    messageEl.innerHTML = `
      <div>${message.content}</div>
      <div class="message-time">${time}</div>
    `;

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

      // Message will be added via the messageReceived event
    } catch (error) {
      console.error('Failed to send message:', error);
      this.showError('Failed to send message. Please try again.');
    }
  }

  private logout() {
    localStorage.removeItem('anychat_token');
    this.currentUserId = null;
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
    // In a real app, use browser notifications or a toast component
  }
}

// Initialize the app
const app = new ChatApp();
app.init().catch(console.error);
