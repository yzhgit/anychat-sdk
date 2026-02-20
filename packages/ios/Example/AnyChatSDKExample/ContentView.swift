import SwiftUI
import AnyChatSDK

@main
struct AnyChatSDKExampleApp: App {
    @StateObject private var appState = AppState()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(appState)
        }
    }
}

class AppState: ObservableObject {
    @Published var client: AnyChatClient?
    @Published var isConnected = false
    @Published var isLoggedIn = false
    @Published var currentUser: UserProfile?
    @Published var errorMessage: String?

    init() {
        setupClient()
    }

    private func setupClient() {
        do {
            let documentsPath = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
            let dbPath = documentsPath + "/anychat_example.db"

            let config = ClientConfig(
                // Local development server (change to your machine's IP)
                gatewayURL: "ws://192.168.2.100:8080",
                apiBaseURL: "http://192.168.2.100:8080/api/v1",
                deviceId: UIDevice.current.identifierForVendor?.uuidString ?? "unknown",
                dbPath: dbPath
            )

            client = try AnyChatClient(config: config)

            // Start monitoring connection state
            Task {
                await monitorConnectionState()
            }
        } catch {
            errorMessage = "Failed to initialize client: \(error.localizedDescription)"
        }
    }

    @MainActor
    private func monitorConnectionState() async {
        guard let client = client else { return }

        for await state in client.connectionState {
            switch state {
            case .connected:
                isConnected = true
            case .disconnected, .connecting, .reconnecting:
                isConnected = false
            }
        }
    }

    func connect() {
        client?.connect()
    }

    func disconnect() {
        client?.disconnect()
    }

    func login(account: String, password: String) async throws {
        guard let client = client else {
            throw AnyChatError.invalidState
        }

        _ = try await client.auth.login(account: account, password: password)
        currentUser = try await client.user.getProfile()
        isLoggedIn = true
    }

    func logout() async throws {
        guard let client = client else { return }
        try await client.auth.logout()
        isLoggedIn = false
        currentUser = nil
    }
}

struct ContentView: View {
    @EnvironmentObject var appState: AppState
    @State private var selectedTab = 0

    var body: some View {
        if !appState.isLoggedIn {
            LoginView()
        } else {
            TabView(selection: $selectedTab) {
                ConversationsView()
                    .tabItem {
                        Label("Chats", systemImage: "message.fill")
                    }
                    .tag(0)

                FriendsView()
                    .tabItem {
                        Label("Friends", systemImage: "person.2.fill")
                    }
                    .tag(1)

                GroupsView()
                    .tabItem {
                        Label("Groups", systemImage: "person.3.fill")
                    }
                    .tag(2)

                ProfileView()
                    .tabItem {
                        Label("Profile", systemImage: "person.circle.fill")
                    }
                    .tag(3)
            }
        }
    }
}

// MARK: - Login View

struct LoginView: View {
    @EnvironmentObject var appState: AppState
    @State private var account = ""
    @State private var password = ""
    @State private var isLoggingIn = false
    @State private var errorMessage: String?

    var body: some View {
        NavigationView {
            VStack(spacing: 20) {
                Image(systemName: "bubble.left.and.bubble.right.fill")
                    .resizable()
                    .aspectRatio(contentMode: .fit)
                    .frame(width: 100, height: 100)
                    .foregroundColor(.blue)

                Text("AnyChat SDK Example")
                    .font(.title)
                    .fontWeight(.bold)

                VStack(spacing: 15) {
                    TextField("Account", text: $account)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .autocapitalization(.none)
                        .disableAutocorrection(true)

                    SecureField("Password", text: $password)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                }
                .padding(.horizontal)

                if let errorMessage = errorMessage {
                    Text(errorMessage)
                        .foregroundColor(.red)
                        .font(.caption)
                }

                Button(action: login) {
                    if isLoggingIn {
                        ProgressView()
                            .progressViewStyle(CircularProgressViewStyle(tint: .white))
                    } else {
                        Text("Login")
                            .fontWeight(.semibold)
                    }
                }
                .frame(maxWidth: .infinity)
                .padding()
                .background(Color.blue)
                .foregroundColor(.white)
                .cornerRadius(10)
                .padding(.horizontal)
                .disabled(isLoggingIn || account.isEmpty || password.isEmpty)

                Button("Register") {
                    // TODO: Navigate to registration view
                }
                .foregroundColor(.blue)

                Spacer()

                HStack {
                    Circle()
                        .fill(appState.isConnected ? Color.green : Color.red)
                        .frame(width: 10, height: 10)
                    Text(appState.isConnected ? "Connected" : "Disconnected")
                        .font(.caption)
                        .foregroundColor(.gray)
                }
            }
            .padding()
            .navigationBarHidden(true)
            .onAppear {
                appState.connect()
            }
        }
    }

    private func login() {
        isLoggingIn = true
        errorMessage = nil

        Task {
            do {
                try await appState.login(account: account, password: password)
            } catch {
                errorMessage = "Login failed: \(error.localizedDescription)"
            }
            isLoggingIn = false
        }
    }
}

// MARK: - Conversations View

struct ConversationsView: View {
    @EnvironmentObject var appState: AppState
    @State private var conversations: [Conversation] = []
    @State private var isLoading = true

    var body: some View {
        NavigationView {
            Group {
                if isLoading {
                    ProgressView("Loading conversations...")
                } else if conversations.isEmpty {
                    VStack {
                        Image(systemName: "message.badge")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 80, height: 80)
                            .foregroundColor(.gray)
                        Text("No conversations yet")
                            .foregroundColor(.gray)
                    }
                } else {
                    List(conversations, id: \.conversationId) { conversation in
                        NavigationLink(destination: ChatView(conversation: conversation)) {
                            ConversationRow(conversation: conversation)
                        }
                    }
                }
            }
            .navigationTitle("Chats")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button(action: loadConversations) {
                        Image(systemName: "arrow.clockwise")
                    }
                }
            }
            .onAppear(perform: loadConversations)
        }
    }

    private func loadConversations() {
        guard let client = appState.client else { return }

        isLoading = true
        Task {
            do {
                conversations = try await client.conversation.getList()
            } catch {
                print("Failed to load conversations: \(error)")
            }
            isLoading = false
        }
    }
}

struct ConversationRow: View {
    let conversation: Conversation

    var body: some View {
        HStack {
            Image(systemName: "person.circle.fill")
                .resizable()
                .frame(width: 50, height: 50)
                .foregroundColor(.blue)

            VStack(alignment: .leading, spacing: 4) {
                Text(conversation.conversationId)
                    .fontWeight(.semibold)

                Text(conversation.lastMessage ?? "No messages")
                    .font(.caption)
                    .foregroundColor(.gray)
                    .lineLimit(1)
            }

            Spacer()

            if conversation.unreadCount > 0 {
                Text("\(conversation.unreadCount)")
                    .font(.caption)
                    .fontWeight(.bold)
                    .foregroundColor(.white)
                    .padding(6)
                    .background(Color.red)
                    .clipShape(Circle())
            }
        }
        .padding(.vertical, 4)
    }
}

// MARK: - Chat View

struct ChatView: View {
    @EnvironmentObject var appState: AppState
    let conversation: Conversation
    @State private var messages: [Message] = []
    @State private var newMessageText = ""
    @State private var isSending = false

    var body: some View {
        VStack {
            ScrollView {
                LazyVStack {
                    ForEach(messages, id: \.messageId) { message in
                        MessageBubble(message: message, isCurrentUser: message.senderId == appState.currentUser?.userId)
                    }
                }
            }

            HStack {
                TextField("Type a message...", text: $newMessageText)
                    .textFieldStyle(RoundedBorderTextFieldStyle())

                Button(action: sendMessage) {
                    if isSending {
                        ProgressView()
                    } else {
                        Image(systemName: "paperplane.fill")
                    }
                }
                .disabled(newMessageText.isEmpty || isSending)
            }
            .padding()
        }
        .navigationTitle("Chat")
        .navigationBarTitleDisplayMode(.inline)
        .onAppear(perform: loadMessages)
    }

    private func loadMessages() {
        guard let client = appState.client else { return }

        Task {
            do {
                messages = try await client.message.getHistory(sessionId: conversation.conversationId, limit: 50)
            } catch {
                print("Failed to load messages: \(error)")
            }
        }
    }

    private func sendMessage() {
        guard let client = appState.client else { return }
        guard !newMessageText.isEmpty else { return }

        let messageText = newMessageText
        newMessageText = ""
        isSending = true

        Task {
            do {
                try await client.message.sendText(sessionId: conversation.conversationId, content: messageText)
                await loadMessages()
            } catch {
                print("Failed to send message: \(error)")
            }
            isSending = false
        }
    }
}

struct MessageBubble: View {
    let message: Message
    let isCurrentUser: Bool

    var body: some View {
        HStack {
            if isCurrentUser { Spacer() }

            Text(message.content)
                .padding(10)
                .background(isCurrentUser ? Color.blue : Color.gray.opacity(0.2))
                .foregroundColor(isCurrentUser ? .white : .primary)
                .cornerRadius(15)

            if !isCurrentUser { Spacer() }
        }
        .padding(.horizontal)
        .padding(.vertical, 2)
    }
}

// MARK: - Friends View

struct FriendsView: View {
    @EnvironmentObject var appState: AppState
    @State private var friends: [Friend] = []
    @State private var isLoading = true

    var body: some View {
        NavigationView {
            Group {
                if isLoading {
                    ProgressView("Loading friends...")
                } else if friends.isEmpty {
                    VStack {
                        Image(systemName: "person.2")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 80, height: 80)
                            .foregroundColor(.gray)
                        Text("No friends yet")
                            .foregroundColor(.gray)
                    }
                } else {
                    List(friends, id: \.userInfo.userId) { friend in
                        HStack {
                            Image(systemName: "person.circle.fill")
                                .resizable()
                                .frame(width: 40, height: 40)
                                .foregroundColor(.blue)

                            VStack(alignment: .leading) {
                                Text(friend.userInfo.username)
                                    .fontWeight(.semibold)

                                if let remark = friend.remark {
                                    Text(remark)
                                        .font(.caption)
                                        .foregroundColor(.gray)
                                }
                            }

                            Spacer()
                        }
                        .padding(.vertical, 4)
                    }
                }
            }
            .navigationTitle("Friends")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button(action: loadFriends) {
                        Image(systemName: "arrow.clockwise")
                    }
                }
            }
            .onAppear(perform: loadFriends)
        }
    }

    private func loadFriends() {
        guard let client = appState.client else { return }

        isLoading = true
        Task {
            do {
                friends = try await client.friend.getList()
            } catch {
                print("Failed to load friends: \(error)")
            }
            isLoading = false
        }
    }
}

// MARK: - Groups View

struct GroupsView: View {
    @EnvironmentObject var appState: AppState
    @State private var groups: [Group] = []
    @State private var isLoading = true

    var body: some View {
        NavigationView {
            Group {
                if isLoading {
                    ProgressView("Loading groups...")
                } else if groups.isEmpty {
                    VStack {
                        Image(systemName: "person.3")
                            .resizable()
                            .aspectRatio(contentMode: .fit)
                            .frame(width: 80, height: 80)
                            .foregroundColor(.gray)
                        Text("No groups yet")
                            .foregroundColor(.gray)
                    }
                } else {
                    List(groups, id: \.groupId) { group in
                        HStack {
                            Image(systemName: "person.3.fill")
                                .resizable()
                                .aspectRatio(contentMode: .fit)
                                .frame(width: 40, height: 40)
                                .foregroundColor(.green)

                            VStack(alignment: .leading) {
                                Text(group.name)
                                    .fontWeight(.semibold)

                                Text("\(group.memberCount) members")
                                    .font(.caption)
                                    .foregroundColor(.gray)
                            }

                            Spacer()
                        }
                        .padding(.vertical, 4)
                    }
                }
            }
            .navigationTitle("Groups")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button(action: loadGroups) {
                        Image(systemName: "arrow.clockwise")
                    }
                }
            }
            .onAppear(perform: loadGroups)
        }
    }

    private func loadGroups() {
        guard let client = appState.client else { return }

        isLoading = true
        Task {
            do {
                groups = try await client.group.getList()
            } catch {
                print("Failed to load groups: \(error)")
            }
            isLoading = false
        }
    }
}

// MARK: - Profile View

struct ProfileView: View {
    @EnvironmentObject var appState: AppState

    var body: some View {
        NavigationView {
            List {
                Section {
                    if let user = appState.currentUser {
                        HStack {
                            Image(systemName: "person.circle.fill")
                                .resizable()
                                .frame(width: 60, height: 60)
                                .foregroundColor(.blue)

                            VStack(alignment: .leading, spacing: 4) {
                                Text(user.nickname ?? user.username)
                                    .font(.headline)
                                Text(user.username)
                                    .font(.caption)
                                    .foregroundColor(.gray)
                            }

                            Spacer()
                        }
                        .padding(.vertical, 8)
                    }
                }

                Section {
                    HStack {
                        Image(systemName: "circle.fill")
                            .foregroundColor(appState.isConnected ? .green : .red)
                        Text("Connection Status")
                        Spacer()
                        Text(appState.isConnected ? "Connected" : "Disconnected")
                            .foregroundColor(.gray)
                    }
                }

                Section {
                    Button(action: logout) {
                        HStack {
                            Image(systemName: "rectangle.portrait.and.arrow.right")
                            Text("Logout")
                        }
                        .foregroundColor(.red)
                    }
                }
            }
            .navigationTitle("Profile")
        }
    }

    private func logout() {
        Task {
            do {
                try await appState.logout()
                appState.disconnect()
            } catch {
                print("Logout failed: \(error)")
            }
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
            .environmentObject(AppState())
    }
}
