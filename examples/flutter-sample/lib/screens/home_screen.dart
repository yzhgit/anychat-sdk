import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:flutter_sample/services/auth_service.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  Future<void> _handleLogout(BuildContext context) async {
    // Show confirmation dialog
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Logout'),
        content: const Text('Are you sure you want to logout?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () => Navigator.of(context).pop(true),
            child: const Text('Logout'),
          ),
        ],
      ),
    );

    if (confirmed != true) return;
    if (!context.mounted) return;

    try {
      final authService = context.read<AuthService>();
      await authService.logout();

      if (!context.mounted) return;

      // Navigate back to welcome screen
      Navigator.of(context).pushReplacementNamed('/');
    } catch (e) {
      if (!context.mounted) return;

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Logout failed: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('AnyChat'),
        automaticallyImplyLeading: false,
        actions: [
          // Connection status indicator
          Consumer<AuthService>(
            builder: (context, authService, child) {
              final isConnected = authService.isConnected;
              return Padding(
                padding: const EdgeInsets.symmetric(horizontal: 16),
                child: Center(
                  child: Row(
                    children: [
                      Icon(
                        Icons.circle,
                        size: 12,
                        color: isConnected ? Colors.green : Colors.grey,
                      ),
                      const SizedBox(width: 8),
                      Text(
                        isConnected ? 'Connected' : 'Disconnected',
                        style: TextStyle(
                          color: isConnected ? Colors.green : Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ),
              );
            },
          ),
          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () => _handleLogout(context),
            tooltip: 'Logout',
          ),
        ],
      ),
      body: Consumer<AuthService>(
        builder: (context, authService, child) {
          final token = authService.currentToken;
          final userId = authService.userId;

          return Center(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(32),
              child: Card(
                elevation: 4,
                child: Container(
                  constraints: const BoxConstraints(maxWidth: 600),
                  padding: const EdgeInsets.all(32),
                  child: Column(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      // User icon
                      CircleAvatar(
                        radius: 50,
                        backgroundColor: Theme.of(context).primaryColor,
                        child: const Icon(
                          Icons.person,
                          size: 50,
                          color: Colors.white,
                        ),
                      ),
                      const SizedBox(height: 24),

                      // Welcome message
                      Text(
                        'Welcome!',
                        style: Theme.of(context).textTheme.headlineSmall,
                      ),
                      const SizedBox(height: 8),
                      if (userId != null)
                        Text(
                          'User ID: $userId',
                          style: Theme.of(context).textTheme.bodyMedium,
                        ),
                      const SizedBox(height: 32),

                      // Session info
                      Container(
                        padding: const EdgeInsets.all(16),
                        decoration: BoxDecoration(
                          color: Colors.grey.shade100,
                          borderRadius: BorderRadius.circular(8),
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              'Session Information',
                              style: Theme.of(context).textTheme.titleMedium,
                            ),
                            const SizedBox(height: 12),
                            _buildInfoRow(
                              'Status',
                              authService.isLoggedIn ? 'Logged In' : 'Logged Out',
                              authService.isLoggedIn ? Colors.green : Colors.red,
                            ),
                            const Divider(height: 16),
                            _buildInfoRow(
                              'Connection',
                              authService.connectionState.toString().split('.').last,
                              authService.isConnected ? Colors.green : Colors.orange,
                            ),
                            if (token != null) ...[
                              const Divider(height: 16),
                              _buildInfoRow(
                                'Token Expires',
                                _formatTimestamp(token.expiresAtMs),
                                null,
                              ),
                            ],
                          ],
                        ),
                      ),
                      const SizedBox(height: 24),

                      // Placeholder for future features
                      const Divider(),
                      const SizedBox(height: 16),
                      Text(
                        'Coming Soon',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      const SizedBox(height: 12),
                      const Text(
                        'More features like messaging, friends, and groups will be available here.',
                        textAlign: TextAlign.center,
                        style: TextStyle(color: Colors.grey),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }

  Widget _buildInfoRow(String label, String value, Color? valueColor) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Text(
          label,
          style: const TextStyle(fontWeight: FontWeight.w500),
        ),
        Text(
          value,
          style: TextStyle(
            color: valueColor,
            fontWeight: valueColor != null ? FontWeight.w500 : null,
          ),
        ),
      ],
    );
  }

  String _formatTimestamp(int timestampMs) {
    final dateTime = DateTime.fromMillisecondsSinceEpoch(timestampMs);
    return '${dateTime.year}-${dateTime.month.toString().padLeft(2, '0')}-${dateTime.day.toString().padLeft(2, '0')} '
        '${dateTime.hour.toString().padLeft(2, '0')}:${dateTime.minute.toString().padLeft(2, '0')}';
  }
}
