# AnyChat Flutter Sample

A Flutter desktop application demonstrating the AnyChat SDK integration with user registration, login, and logout functionality.

## Features

- **User Registration**: Create new accounts with email, username, and password
- **User Login**: Authenticate with existing credentials
- **User Logout**: Securely log out and clear session
- **Connection Status**: Real-time WebSocket connection monitoring
- **Session Management**: Automatic session persistence using SharedPreferences
- **Desktop Support**: Runs on Linux, macOS, and Windows

## Project Structure

```
lib/
├── main.dart                    # Application entry point
├── config.dart                  # API configuration
├── services/
│   └── auth_service.dart        # Authentication service (SDK wrapper)
└── screens/
    ├── welcome_screen.dart      # Landing page
    ├── login_screen.dart        # Login form
    ├── register_screen.dart     # Registration form
    └── home_screen.dart         # Main screen (after login)
```

## Prerequisites

1. **Flutter SDK** 3.0+ installed
2. **AnyChat Server** running and accessible
3. **Git submodules** initialized:
   ```bash
   cd ../..
   git submodule update --init --recursive
   ```
4. **C++ Core SDK** built:
   ```bash
   cd ../..
   cmake -B build
   cmake --build build
   ```

## Configuration

Edit `lib/config.dart` to point to your AnyChat server:

```dart
class AppConfig {
  static const String gatewayUrl = 'wss://your-server.com';
  static const String apiBaseUrl = 'https://your-server.com/api/v1';
  static const String deviceId = 'flutter-desktop-sample';
}
```

## Running the Application

### Linux

```bash
flutter run -d linux
```

Requirements:
- GTK 3.0+
- CMake 3.10+
- Ninja build system
- Clang or GCC

Install dependencies (Ubuntu/Debian):
```bash
sudo apt-get install libgtk-3-dev cmake ninja-build clang
```

### macOS

```bash
flutter run -d macos
```

Requirements:
- Xcode 14+
- CocoaPods

### Windows

```bash
flutter run -d windows
```

Requirements:
- Visual Studio 2019+ with C++ development tools
- CMake 3.14+

## Usage Flow

1. **Launch App** → SDK initializes and connects to gateway
2. **Welcome Screen** → Choose "Login" or "Create Account"
3. **Register** → Enter email, username, and password
   - Password must be at least 6 characters
   - Username must be at least 3 characters
4. **Login** → Enter account and password
5. **Home Screen** → View session info, logout

## Key Components

### AuthService

The `AuthService` class wraps the AnyChat SDK and provides:
- SDK initialization
- User registration
- User login/logout
- Session persistence (access token, refresh token)
- Connection state monitoring
- Error handling

### Screens

- **WelcomeScreen**: Landing page with connection status
- **LoginScreen**: Form-based authentication
- **RegisterScreen**: New user registration form
- **HomeScreen**: Authenticated user view with session info

## Building for Release

### Linux

```bash
flutter build linux --release
```

Output: `build/linux/x64/release/bundle/`

### macOS

```bash
flutter build macos --release
```

Output: `build/macos/Build/Products/Release/flutter_sample.app`

### Windows

```bash
flutter build windows --release
```

Output: `build/windows/runner/Release/`

## Troubleshooting

### SDK Initialization Fails

- Ensure submodules are initialized: `git submodule update --init --recursive`
- Verify C++ core SDK is built: Check `../../build/core/libanychat_c.a` exists
- Check server URLs in `config.dart`

### Connection Issues

- Verify AnyChat server is running and accessible
- Check firewall/network settings
- Ensure gateway URL uses `wss://` (not `ws://`)

### Build Errors

**Linux**:
```bash
# Install GTK development headers
sudo apt-get install libgtk-3-dev
```

**macOS**:
```bash
# Ensure Xcode command line tools are installed
xcode-select --install
```

**Windows**:
- Ensure Visual Studio C++ tools are installed
- Run from "Developer Command Prompt for VS"

## Testing Credentials

For development/testing, you can use:
- Email: `test@example.com`
- Password: `password123`

**Important**: Change these in production!

## Next Steps

This sample demonstrates basic authentication. Additional features to implement:

- [ ] Friend management (add, remove, list friends)
- [ ] Messaging (send, receive, history)
- [ ] Conversations (list, create, delete)
- [ ] Groups (create, join, manage members)
- [ ] User profile editing
- [ ] File uploads (avatar, attachments)
- [ ] Real-time event notifications

## License

MIT License - see [../../LICENSE](../../LICENSE)

## Links

- **Backend API**: [AnyChat Server](https://github.com/yzhgit/anychat-server)
- **SDK Documentation**: [packages/flutter/README.md](../../packages/flutter/README.md)
- **Issues**: [GitHub Issues](https://github.com/yzhgit/anychat-sdk/issues)
