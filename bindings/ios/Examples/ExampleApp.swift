//
//  ExampleApp.swift
//  AnyChatSDK Example
//
//  Example usage of AnyChatSDK in a SwiftUI app
//

import SwiftUI
import AnyChatSDK

@main
struct AnyChatExampleApp: App {
    @StateObject private var chatManager = ChatManager()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(chatManager)
        }
    }
}

// MARK: - Chat Manager

@MainActor
class ChatManager: ObservableObject {
    @Published var connectionState: ConnectionState = .disconnected
    @Published var isLoggedIn = false
    @Published var conversations: [Conversation] = []
    @Published var messages: [String: [Message]] = [:]
    @Published var error: String?

    private var client: AnyChatClient?
    private var eventTasks: [Task<Void, Never>] = []

    func initialize() {
        do {
            let config = ClientConfig(
                gatewayURL: "wss://api.anychat.io",
                apiBaseURL: "https://api.anychat.io/api/v1",
                deviceId: UIDevice.current.identifierForVendor?.uuidString ?? "unknown",
                dbPath: getDocumentsDirectory().appendingPathComponent("anychat.db").path
            )

            client = try AnyChatClient(config: config)
            setupEventListeners()
            client?.connect()
        } catch {
            self.error = "Failed to initialize: \(error.localizedDescription)"
        }
    }

    func login(account: String, password: String) async {
        guard let client = client else { return }

        do {
            let token = try await client.auth.login(
                account: account,
                password: password
            )
            isLoggedIn = true
            print("Logged in successfully")

            // Load conversations after login
            await loadConversations()
        } catch {
            self.error = "Login failed: \(error.localizedDescription)"
        }
    }

    func logout() async {
        guard let client = client else { return }

        do {
            try await client.auth.logout()
            isLoggedIn = false
            conversations = []
            messages = [:]
        } catch {
            self.error = "Logout failed: \(error.localizedDescription)"
        }
    }

    func loadConversations() async {
        guard let client = client else { return }

        do {
            let convs = try await client.conversation.getList()
            conversations = convs
        } catch {
            self.error = "Failed to load conversations: \(error.localizedDescription)"
        }
    }

    func loadMessages(for conversationId: String) async {
        guard let client = client else { return }

        do {
            let msgs = try await client.message.getHistory(
                sessionId: conversationId,
                limit: 50
            )
            messages[conversationId] = msgs
        } catch {
            self.error = "Failed to load messages: \(error.localizedDescription)"
        }
    }

    func sendMessage(to conversationId: String, content: String) async {
        guard let client = client else { return }

        do {
            try await client.message.sendText(
                sessionId: conversationId,
                content: content
            )
            // Reload messages to show the sent message
            await loadMessages(for: conversationId)
        } catch {
            self.error = "Failed to send message: \(error.localizedDescription)"
        }
    }

    private func setupEventListeners() {
        guard let client = client else { return }

        // Connection state
        let connectionTask = Task {
            for await state in client.connectionState {
                await MainActor.run {
                    connectionState = state
                }
            }
        }
        eventTasks.append(connectionTask)

        // Incoming messages
        let messagesTask = Task {
            for await message in client.message.receivedMessages {
                await MainActor.run {
                    if var msgs = messages[message.conversationId] {
                        msgs.append(message)
                        messages[message.conversationId] = msgs
                    }
                }
            }
        }
        eventTasks.append(messagesTask)

        // Conversation updates
        let conversationTask = Task {
            for await conversation in client.conversation.conversationUpdated {
                await MainActor.run {
                    if let index = conversations.firstIndex(where: { $0.conversationId == conversation.conversationId }) {
                        conversations[index] = conversation
                    } else {
                        conversations.append(conversation)
                    }
                }
            }
        }
        eventTasks.append(conversationTask)
    }

    private func getDocumentsDirectory() -> URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
    }

    deinit {
        eventTasks.forEach { $0.cancel() }
    }
}

// MARK: - Content View

struct ContentView: View {
    @EnvironmentObject var chatManager: ChatManager

    var body: some View {
        NavigationView {
            if chatManager.isLoggedIn {
                ConversationListView()
            } else {
                LoginView()
            }
        }
        .onAppear {
            chatManager.initialize()
        }
        .alert("Error", isPresented: .constant(chatManager.error != nil)) {
            Button("OK") {
                chatManager.error = nil
            }
        } message: {
            Text(chatManager.error ?? "")
        }
    }
}

// MARK: - Login View

struct LoginView: View {
    @EnvironmentObject var chatManager: ChatManager
    @State private var account = ""
    @State private var password = ""
    @State private var isLoggingIn = false

    var body: some View {
        VStack(spacing: 20) {
            Text("AnyChat Login")
                .font(.largeTitle)
                .padding(.bottom, 40)

            TextField("Account", text: $account)
                .textFieldStyle(.roundedBorder)
                .autocapitalization(.none)
                .padding(.horizontal)

            SecureField("Password", text: $password)
                .textFieldStyle(.roundedBorder)
                .padding(.horizontal)

            Button(action: login) {
                if isLoggingIn {
                    ProgressView()
                        .progressViewStyle(.circular)
                } else {
                    Text("Login")
                        .frame(maxWidth: .infinity)
                }
            }
            .buttonStyle(.borderedProminent)
            .disabled(account.isEmpty || password.isEmpty || isLoggingIn)
            .padding(.horizontal)
        }
        .padding()
    }

    private func login() {
        isLoggingIn = true
        Task {
            await chatManager.login(account: account, password: password)
            isLoggingIn = false
        }
    }
}

// MARK: - Conversation List View

struct ConversationListView: View {
    @EnvironmentObject var chatManager: ChatManager

    var body: some View {
        List {
            ForEach(chatManager.conversations, id: \.conversationId) { conversation in
                NavigationLink(destination: ChatView(conversation: conversation)) {
                    ConversationRow(conversation: conversation)
                }
            }
        }
        .navigationTitle("Conversations")
        .toolbar {
            ToolbarItem(placement: .navigationBarTrailing) {
                Button("Logout") {
                    Task {
                        await chatManager.logout()
                    }
                }
            }
        }
        .refreshable {
            await chatManager.loadConversations()
        }
    }
}

struct ConversationRow: View {
    let conversation: Conversation

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(conversation.targetId)
                    .font(.headline)
                Spacer()
                Text(conversation.lastMessageTime, style: .time)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            HStack {
                Text(conversation.lastMessageText)
                    .font(.subheadline)
                    .foregroundColor(.secondary)
                    .lineLimit(1)
                Spacer()
                if conversation.unreadCount > 0 {
                    Text("\(conversation.unreadCount)")
                        .font(.caption)
                        .padding(4)
                        .background(Color.red)
                        .foregroundColor(.white)
                        .clipShape(Circle())
                }
            }
        }
        .padding(.vertical, 4)
    }
}

// MARK: - Chat View

struct ChatView: View {
    @EnvironmentObject var chatManager: ChatManager
    let conversation: Conversation

    @State private var messageText = ""
    @State private var messages: [Message] = []

    var body: some View {
        VStack {
            ScrollView {
                LazyVStack(spacing: 8) {
                    ForEach(messages, id: \.messageId) { message in
                        MessageBubble(message: message)
                    }
                }
                .padding()
            }

            HStack {
                TextField("Type a message...", text: $messageText)
                    .textFieldStyle(.roundedBorder)

                Button(action: sendMessage) {
                    Image(systemName: "arrow.up.circle.fill")
                        .font(.title2)
                }
                .disabled(messageText.isEmpty)
            }
            .padding()
        }
        .navigationTitle(conversation.targetId)
        .navigationBarTitleDisplayMode(.inline)
        .onAppear {
            loadMessages()
        }
        .onChange(of: chatManager.messages[conversation.conversationId]) { newMessages in
            if let msgs = newMessages {
                messages = msgs
            }
        }
    }

    private func loadMessages() {
        Task {
            await chatManager.loadMessages(for: conversation.conversationId)
            if let msgs = chatManager.messages[conversation.conversationId] {
                messages = msgs
            }
        }
    }

    private func sendMessage() {
        let text = messageText
        messageText = ""

        Task {
            await chatManager.sendMessage(to: conversation.conversationId, content: text)
        }
    }
}

struct MessageBubble: View {
    let message: Message

    var body: some View {
        HStack {
            if message.senderId == "me" {
                Spacer()
            }

            VStack(alignment: message.senderId == "me" ? .trailing : .leading, spacing: 4) {
                Text(message.content)
                    .padding(10)
                    .background(message.senderId == "me" ? Color.blue : Color.gray.opacity(0.2))
                    .foregroundColor(message.senderId == "me" ? .white : .primary)
                    .cornerRadius(16)

                Text(message.timestamp, style: .time)
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }

            if message.senderId != "me" {
                Spacer()
            }
        }
    }
}

// MARK: - Preview

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
            .environmentObject(ChatManager())
    }
}
