package com.anychat.example

import android.os.Bundle
import android.provider.Settings
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.anychat.sdk.AnyChatClient
import com.anychat.sdk.ClientConfig
import com.anychat.sdk.models.ConnectionState
import com.anychat.sdk.models.MessageType
import kotlinx.coroutines.launch

/**
 * Simple example demonstrating AnyChat SDK usage
 */
class MainActivity : AppCompatActivity() {

    private lateinit var client: AnyChatClient

    companion object {
        private const val TAG = "AnyChatExample"

        // Local development server (change to your machine's IP)
        private const val GATEWAY_URL = "ws://192.168.2.100:8080"
        private const val API_BASE_URL = "http://192.168.2.100:8080/api/v1"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initializeSDK()
        setupConnectionListener()
        runExamples()
    }

    private fun initializeSDK() {
        val config = ClientConfig(
            gatewayUrl = GATEWAY_URL,
            apiBaseUrl = API_BASE_URL,
            deviceId = getDeviceId(),
            dbPath = getDatabasePath("anychat.db").absolutePath,
            connectTimeoutMs = 10_000,
            maxReconnectAttempts = 5,
            autoReconnect = true
        )

        client = AnyChatClient(config)
        Log.d(TAG, "AnyChat SDK initialized")
    }

    private fun setupConnectionListener() {
        lifecycleScope.launch {
            client.connectionStateFlow.collect { state ->
                when (state) {
                    ConnectionState.CONNECTED -> {
                        Log.d(TAG, "✅ WebSocket connected")
                        showToast("Connected to AnyChat")
                    }
                    ConnectionState.CONNECTING -> {
                        Log.d(TAG, "🔄 Connecting to WebSocket...")
                        showToast("Connecting...")
                    }
                    ConnectionState.DISCONNECTED -> {
                        Log.w(TAG, "⚠️ Disconnected from WebSocket")
                        showToast("Disconnected")
                    }
                    ConnectionState.FAILED -> {
                        Log.e(TAG, "❌ Connection failed")
                        showToast("Connection failed")
                    }
                }
            }
        }
    }

    private fun runExamples() {
        lifecycleScope.launch {
            try {
                // Example 1: Register and login
                exampleAuthentication()

                // Example 2: Search and add friends
                exampleFriendManagement()

                // Example 3: Send messages
                exampleMessaging()

                // Example 4: Group chat
                exampleGroupChat()

            } catch (e: Exception) {
                Log.e(TAG, "Example failed", e)
                showToast("Error: ${e.message}")
            }
        }
    }

    /**
     * Example 1: User Authentication
     */
    private suspend fun exampleAuthentication() {
        Log.d(TAG, "=== Example 1: Authentication ===")

        // Register a new user
        try {
            val registerResult = client.auth.register(
                username = "demo_user_${System.currentTimeMillis()}",
                password = "SecurePassword123!",
                email = "demo@example.com",
                nickname = "Demo User"
            )
            Log.d(TAG, "✅ User registered: ${registerResult.user.id}")
            showToast("Registered successfully")
        } catch (e: Exception) {
            Log.w(TAG, "Registration failed (may already exist): ${e.message}")
        }

        // Login
        val loginResult = client.auth.login(
            username = "demo_user",
            password = "SecurePassword123!"
        )
        Log.d(TAG, "✅ Login successful")
        Log.d(TAG, "   User ID: ${loginResult.user.id}")
        Log.d(TAG, "   Token: ${loginResult.token.take(20)}...")
        showToast("Logged in successfully")

        // Connect to WebSocket after successful login
        client.connect()
    }

    /**
     * Example 2: Friend Management
     */
    private suspend fun exampleFriendManagement() {
        Log.d(TAG, "=== Example 2: Friend Management ===")

        // Search for users
        val users = client.friend.searchUsers(
            query = "john",
            page = 1,
            pageSize = 10
        )
        Log.d(TAG, "✅ Found ${users.total} users matching 'john'")
        users.list.forEach { user ->
            Log.d(TAG, "   - ${user.username} (${user.nickname})")
        }

        // Send friend request (if users found)
        if (users.list.isNotEmpty()) {
            val targetUser = users.list.first()
            client.friend.sendRequest(
                userId = targetUser.id,
                message = "Hi! Let's connect on AnyChat"
            )
            Log.d(TAG, "✅ Friend request sent to ${targetUser.username}")
            showToast("Friend request sent")
        }

        // Get friend list
        val friends = client.friend.getFriendList()
        Log.d(TAG, "✅ You have ${friends.total} friends")
        friends.list.forEach { friend ->
            Log.d(TAG, "   - ${friend.username} (${friend.nickname})")
        }
    }

    /**
     * Example 3: Messaging
     */
    private suspend fun exampleMessaging() {
        Log.d(TAG, "=== Example 3: Messaging ===")

        // Get conversation list
        val conversations = client.conversation.getConversationList(
            page = 1,
            pageSize = 20
        )
        Log.d(TAG, "✅ You have ${conversations.total} conversations")

        // Send a message to the first conversation
        if (conversations.list.isNotEmpty()) {
            val conv = conversations.list.first()

            val message = client.message.send(
                conversationId = conv.id,
                content = "Hello from AnyChat Android SDK! 👋",
                messageType = MessageType.TEXT
            )
            Log.d(TAG, "✅ Message sent: ${message.id}")
            showToast("Message sent")

            // Get message history
            val history = client.message.getHistory(
                conversationId = conv.id,
                page = 1,
                pageSize = 50
            )
            Log.d(TAG, "✅ Loaded ${history.list.size} messages from conversation")
        }

        // Listen for incoming messages
        lifecycleScope.launch {
            client.message.messageFlow.collect { message ->
                Log.d(TAG, "📩 New message: ${message.content}")
                showToast("New message: ${message.content}")
            }
        }
    }

    /**
     * Example 4: Group Chat
     */
    private suspend fun exampleGroupChat() {
        Log.d(TAG, "=== Example 4: Group Chat ===")

        // Create a new group
        val group = client.group.create(
            name = "Android Developers",
            description = "Group for Android SDK users",
            avatar = null
        )
        Log.d(TAG, "✅ Group created: ${group.name} (${group.id})")
        showToast("Group created")

        // Get friend list to add members
        val friends = client.friend.getFriendList()

        if (friends.list.isNotEmpty()) {
            // Add first 2 friends to the group
            val friendIds = friends.list.take(2).map { it.id }
            client.group.addMember(
                groupId = group.id,
                userIds = friendIds
            )
            Log.d(TAG, "✅ Added ${friendIds.size} members to group")
        }

        // Send a message to the group
        val groupMessage = client.message.send(
            conversationId = group.conversationId,
            content = "Welcome to the Android Developers group! 🎉",
            messageType = MessageType.TEXT
        )
        Log.d(TAG, "✅ Group message sent: ${groupMessage.id}")

        // List group members
        val members = client.group.getMembers(
            groupId = group.id,
            page = 1,
            pageSize = 50
        )
        Log.d(TAG, "✅ Group has ${members.total} members:")
        members.list.forEach { member ->
            Log.d(TAG, "   - ${member.user.username} (${member.role})")
        }
    }

    private fun getDeviceId(): String {
        return Settings.Secure.getString(
            contentResolver,
            Settings.Secure.ANDROID_ID
        ) ?: "unknown_device"
    }

    private fun showToast(message: String) {
        runOnUiThread {
            Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        // Cleanup
        lifecycleScope.launch {
            try {
                client.disconnect()
                Log.d(TAG, "Disconnected from AnyChat")
            } catch (e: Exception) {
                Log.e(TAG, "Error during cleanup", e)
            }
        }
    }
}
