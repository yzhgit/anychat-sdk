import 'package:flutter/material.dart';
import 'package:anychat_sdk/anychat_sdk.dart';
import 'package:path_provider/path_provider.dart';
import 'package:uuid/uuid.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'AnyChat SDK Example',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: const HomePage(),
    );
  }
}

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  AnyChatClient? _client;
  ConnectionState _connectionState = ConnectionState.disconnected;
  bool _isLoggedIn = false;
  final List<String> _logs = [];

  final _accountController = TextEditingController(text: 'user@example.com');
  final _passwordController = TextEditingController(text: 'password');
  final _gatewayController = TextEditingController(text: 'wss://api.anychat.io');
  final _apiController = TextEditingController(text: 'https://api.anychat.io/api/v1');

  @override
  void initState() {
    super.initState();
    _initClient();
  }

  Future<void> _initClient() async {
    try {
      // Get database path
      final appDir = await getApplicationSupportDirectory();
      final dbPath = '${appDir.path}/anychat.db';

      // Generate or load device ID
      const uuid = Uuid();
      final deviceId = uuid.v4();

      _log('Initializing client...');
      _log('DB Path: $dbPath');
      _log('Device ID: $deviceId');

      // Create client
      _client = AnyChatClient(
        gatewayUrl: _gatewayController.text,
        apiBaseUrl: _apiController.text,
        deviceId: deviceId,
        dbPath: dbPath,
        connectTimeoutMs: 10000,
        maxReconnectAttempts: 5,
        autoReconnect: true,
      );

      // Listen to connection state
      _client!.connectionStateStream.listen((state) {
        setState(() {
          _connectionState = state;
        });
        _log('Connection state: $state');
      });

      // Listen for incoming messages
      _client!.messageReceivedStream.listen((message) {
        _log('Message received: ${message.senderId}: ${message.content}');
      });

      // Listen for conversation updates
      _client!.conversationUpdatedStream.listen((conversation) {
        _log('Conversation updated: ${conversation.convId} (${conversation.unreadCount} unread)');
      });

      // Listen for friend requests
      _client!.friendRequestStream.listen((request) {
        _log('Friend request from: ${request.fromUserId}');
      });

      // Listen for incoming calls
      _client!.incomingCallStream.listen((call) {
        _log('Incoming call from: ${call.callerId} (${call.callType})');
      });

      _log('Client initialized successfully');
    } catch (e) {
      _log('Error initializing client: $e');
    }
  }

  void _log(String message) {
    setState(() {
      _logs.add('[${DateTime.now().toIso8601String()}] $message');
    });
    print(message);
  }

  void _connect() {
    if (_client == null) {
      _log('Client not initialized');
      return;
    }
    _log('Connecting...');
    _client!.connect();
  }

  void _disconnect() {
    if (_client == null) {
      _log('Client not initialized');
      return;
    }
    _log('Disconnecting...');
    _client!.disconnect();
  }

  Future<void> _login() async {
    if (_client == null) {
      _log('Client not initialized');
      return;
    }

    try {
      _log('Logging in...');
      final token = await _client!.login(
        account: _accountController.text,
        password: _passwordController.text,
      );
      setState(() {
        _isLoggedIn = true;
      });
      _log('Login successful');
      _log('Access token: ${token.accessToken.substring(0, 20)}...');
    } catch (e) {
      _log('Login failed: $e');
    }
  }

  Future<void> _logout() async {
    if (_client == null) {
      _log('Client not initialized');
      return;
    }

    try {
      _log('Logging out...');
      await _client!.logout();
      setState(() {
        _isLoggedIn = false;
      });
      _log('Logout successful');
    } catch (e) {
      _log('Logout failed: $e');
    }
  }

  Future<void> _getConversations() async {
    if (_client == null) {
      _log('Client not initialized');
      return;
    }

    try {
      _log('Fetching conversations...');
      final conversations = await _client!.getConversations();
      _log('Found ${conversations.length} conversations');
      for (final conv in conversations) {
        _log('  - ${conv.convId}: ${conv.lastMsgText} (${conv.unreadCount} unread)');
      }
    } catch (e) {
      _log('Failed to get conversations: $e');
    }
  }

  Future<void> _getFriends() async {
    if (_client == null) {
      _log('Client not initialized');
      return;
    }

    try {
      _log('Fetching friends...');
      final friends = await _client!.getFriends();
      _log('Found ${friends.length} friends');
      for (final friend in friends) {
        _log('  - ${friend.userId}: ${friend.remark}');
      }
    } catch (e) {
      _log('Failed to get friends: $e');
    }
  }

  void _clearLogs() {
    setState(() {
      _logs.clear();
    });
  }

  @override
  void dispose() {
    _client?.dispose();
    _accountController.dispose();
    _passwordController.dispose();
    _gatewayController.dispose();
    _apiController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('AnyChat SDK Example'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Connection status
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Status',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                    const SizedBox(height: 8),
                    Text('Connection: $_connectionState'),
                    Text('Logged in: $_isLoggedIn'),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Configuration
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Configuration',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                    const SizedBox(height: 8),
                    TextField(
                      controller: _accountController,
                      decoration: const InputDecoration(
                        labelText: 'Account',
                        border: OutlineInputBorder(),
                      ),
                    ),
                    const SizedBox(height: 8),
                    TextField(
                      controller: _passwordController,
                      decoration: const InputDecoration(
                        labelText: 'Password',
                        border: OutlineInputBorder(),
                      ),
                      obscureText: true,
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Actions
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                ElevatedButton(
                  onPressed: _connect,
                  child: const Text('Connect'),
                ),
                ElevatedButton(
                  onPressed: _disconnect,
                  child: const Text('Disconnect'),
                ),
                ElevatedButton(
                  onPressed: _login,
                  child: const Text('Login'),
                ),
                ElevatedButton(
                  onPressed: _logout,
                  child: const Text('Logout'),
                ),
                ElevatedButton(
                  onPressed: _getConversations,
                  child: const Text('Get Conversations'),
                ),
                ElevatedButton(
                  onPressed: _getFriends,
                  child: const Text('Get Friends'),
                ),
                ElevatedButton(
                  onPressed: _clearLogs,
                  child: const Text('Clear Logs'),
                ),
              ],
            ),
            const SizedBox(height: 16),

            // Logs
            Expanded(
              child: Card(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Padding(
                      padding: const EdgeInsets.all(16.0),
                      child: Text(
                        'Logs',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                    ),
                    const Divider(height: 1),
                    Expanded(
                      child: ListView.builder(
                        itemCount: _logs.length,
                        itemBuilder: (context, index) {
                          return Padding(
                            padding: const EdgeInsets.symmetric(
                              horizontal: 16.0,
                              vertical: 4.0,
                            ),
                            child: Text(
                              _logs[index],
                              style: const TextStyle(
                                fontSize: 12,
                                fontFamily: 'monospace',
                              ),
                            ),
                          );
                        },
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
