import 'package:flutter/material.dart';
import 'package:anychat_flutter/anychat.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'AnyChat Flutter Demo',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        useMaterial3: true,
      ),
      home: const ChatHomePage(),
    );
  }
}

class ChatHomePage extends StatefulWidget {
  const ChatHomePage({super.key});

  @override
  State<ChatHomePage> createState() => _ChatHomePageState();
}

class _ChatHomePageState extends State<ChatHomePage> {
  late AnyChatClient _client;
  ConnectionState _connectionState = ConnectionState.disconnected;
  bool _isLoggedIn = false;
  final List<Message> _messages = [];

  final _accountController = TextEditingController(text: 'user@example.com');
  final _passwordController = TextEditingController(text: '');
  final _messageController = TextEditingController();

  @override
  void initState() {
    super.initState();
    _initClient();
  }

  void _initClient() {
    try {
      _client = AnyChatClient(
        gatewayUrl: 'wss://api.anychat.io',
        apiBaseUrl: 'https://api.anychat.io/api/v1',
        deviceId: 'flutter-demo-device-001',
        dbPath: null, // In-memory for demo
      );

      // Listen to connection state
      _client.connectionStateStream.listen((state) {
        setState(() {
          _connectionState = state;
        });
      });

      // Listen to incoming messages
      _client.messageReceivedStream.listen((message) {
        setState(() {
          _messages.add(message);
        });
      });
    } catch (e) {
      debugPrint('Failed to create client: $e');
    }
  }

  @override
  void dispose() {
    _client.dispose();
    _accountController.dispose();
    _passwordController.dispose();
    _messageController.dispose();
    super.dispose();
  }

  Future<void> _connect() async {
    try {
      _client.connect();
    } catch (e) {
      _showError('Connect failed: $e');
    }
  }

  Future<void> _disconnect() async {
    try {
      _client.disconnect();
    } catch (e) {
      _showError('Disconnect failed: $e');
    }
  }

  Future<void> _login() async {
    try {
      await _client.login(
        account: _accountController.text,
        password: _passwordController.text,
      );
      setState(() {
        _isLoggedIn = true;
      });
      _showSuccess('Logged in successfully');
    } catch (e) {
      _showError('Login failed: $e');
    }
  }

  Future<void> _logout() async {
    try {
      await _client.logout();
      setState(() {
        _isLoggedIn = false;
      });
      _showSuccess('Logged out successfully');
    } catch (e) {
      _showError('Logout failed: $e');
    }
  }

  Future<void> _sendMessage() async {
    if (_messageController.text.isEmpty) return;

    try {
      await _client.sendTextMessage(
        sessionId: 'demo-conversation-001',
        content: _messageController.text,
      );
      _messageController.clear();
    } catch (e) {
      _showError('Send message failed: $e');
    }
  }

  void _showError(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: Colors.red,
      ),
    );
  }

  void _showSuccess(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: Colors.green,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('AnyChat Flutter Demo'),
        actions: [
          Chip(
            label: Text(_connectionState.name),
            backgroundColor: _connectionState == ConnectionState.connected
                ? Colors.green
                : Colors.grey,
          ),
          const SizedBox(width: 16),
        ],
      ),
      body: Column(
        children: [
          // Connection controls
          Card(
            margin: const EdgeInsets.all(16),
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Row(
                    children: [
                      Expanded(
                        child: ElevatedButton(
                          onPressed: _connectionState == ConnectionState.disconnected
                              ? _connect
                              : null,
                          child: const Text('Connect'),
                        ),
                      ),
                      const SizedBox(width: 8),
                      Expanded(
                        child: ElevatedButton(
                          onPressed: _connectionState != ConnectionState.disconnected
                              ? _disconnect
                              : null,
                          child: const Text('Disconnect'),
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ),

          // Auth controls
          Card(
            margin: const EdgeInsets.symmetric(horizontal: 16),
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                children: [
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
                    obscureText: true,
                    decoration: const InputDecoration(
                      labelText: 'Password',
                      border: OutlineInputBorder(),
                    ),
                  ),
                  const SizedBox(height: 8),
                  Row(
                    children: [
                      Expanded(
                        child: ElevatedButton(
                          onPressed: !_isLoggedIn ? _login : null,
                          child: const Text('Login'),
                        ),
                      ),
                      const SizedBox(width: 8),
                      Expanded(
                        child: ElevatedButton(
                          onPressed: _isLoggedIn ? _logout : null,
                          child: const Text('Logout'),
                        ),
                      ),
                    ],
                  ),
                  if (_isLoggedIn)
                    Padding(
                      padding: const EdgeInsets.only(top: 8),
                      child: Text(
                        'Token: ${_client.currentToken?.accessToken.substring(0, 20)}...',
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                    ),
                ],
              ),
            ),
          ),

          // Messages
          Expanded(
            child: Card(
              margin: const EdgeInsets.all(16),
              child: Column(
                children: [
                  Expanded(
                    child: ListView.builder(
                      itemCount: _messages.length,
                      itemBuilder: (context, index) {
                        final msg = _messages[index];
                        return ListTile(
                          title: Text(msg.content),
                          subtitle: Text(msg.senderId),
                          trailing: Text(
                            msg.timestamp.toString().substring(11, 19),
                          ),
                        );
                      },
                    ),
                  ),
                  const Divider(height: 1),
                  Padding(
                    padding: const EdgeInsets.all(8),
                    child: Row(
                      children: [
                        Expanded(
                          child: TextField(
                            controller: _messageController,
                            decoration: const InputDecoration(
                              hintText: 'Type a message...',
                              border: OutlineInputBorder(),
                            ),
                            onSubmitted: (_) => _sendMessage(),
                          ),
                        ),
                        const SizedBox(width: 8),
                        IconButton(
                          icon: const Icon(Icons.send),
                          onPressed: _sendMessage,
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}
