# Local Development Testing Guide

Quick guide for testing AnyChat SDK with a local server.

## Prerequisites

1. **AnyChat Server** running locally
2. Your machine's **local IP address** (e.g., `192.168.2.100`)
3. Server listening on port `8080` (or your chosen port)

## Find Your Local IP

### Linux/macOS

```bash
# Option 1: Using ip command
ip addr show | grep "inet " | grep -v 127.0.0.1

# Option 2: Using ifconfig
ifconfig | grep "inet " | grep -v 127.0.0.1

# Option 3: Quick way
hostname -I | awk '{print $1}'
```

### Windows

```cmd
# Option 1: Using ipconfig
ipconfig | findstr IPv4

# Option 2: PowerShell
(Get-NetIPAddress -AddressFamily IPv4 -InterfaceAlias "Wi-Fi" -or -InterfaceAlias "Ethernet").IPAddress
```

Example output: `192.168.2.100`

## Server Configuration

### 1. Start AnyChat Server

Make sure your server is:
- ✅ Running on port `8080` (or update examples to match your port)
- ✅ Listening on `0.0.0.0` (all interfaces, not just `127.0.0.1`)
- ✅ Firewall allows incoming connections on port `8080`

Example server start command:
```bash
# If your server supports it
./anychat-server --host 0.0.0.0 --port 8080
```

### 2. Verify Server is Accessible

Test from the same machine:
```bash
# Test HTTP API
curl http://localhost:8080/health

# Test WebSocket (using wscat if installed)
wscat -c ws://localhost:8080/api/v1/ws
```

Test from another device on same network:
```bash
# Replace 192.168.2.100 with YOUR machine's IP
curl http://192.168.2.100:8080/health
```

## Client Configuration

All SDK examples are already configured for local testing at `192.168.2.100:8080`.

**⚠️ Update to YOUR machine's IP:**

### Flutter Example

File: `packages/flutter/example/lib/main.dart` (Line 42-43)

```dart
final _gatewayController = TextEditingController(text: 'ws://YOUR_IP:8080/api/v1/ws');
final _apiController = TextEditingController(text: 'http://YOUR_IP:8080/api/v1');
```

### Android Example

File: `packages/android/example/src/main/java/com/anychat/example/MainActivity.kt` (Line 25-26)

```kotlin
private const val GATEWAY_URL = "ws://YOUR_IP:8080/api/v1/ws"
private const val API_BASE_URL = "http://YOUR_IP:8080/api/v1"
```

### iOS Example

File: `packages/ios/Example/AnyChatSDKExample/ContentView.swift` (Line 33-34)

```swift
let config = ClientConfig(
    gatewayURL: "ws://YOUR_IP:8080/api/v1/ws",
    apiBaseURL: "http://YOUR_IP:8080/api/v1",
    // ...
)
```

### Web Example

File: `packages/web/example/src/app.ts` (Line 16-17)

```typescript
gatewayUrl: 'ws://YOUR_IP:8080/api/v1/ws',
apiBaseUrl: 'http://YOUR_IP:8080/api/v1',
```

## Testing Workflow

### Step 1: Start Server

```bash
cd /path/to/anychat-server
./start-server.sh  # or your server start command
```

Verify server is running:
```bash
curl http://localhost:8080/health
# Expected: {"status": "ok"} or similar
```

### Step 2: Run Client Example

#### Flutter (Linux Desktop)

```bash
cd packages/flutter/example
flutter run -d linux
```

#### Flutter (Android)

```bash
cd packages/flutter/example
flutter run -d <device-id>
```

#### Android (Native)

```bash
cd packages/android
./gradlew :example:installDebug
adb logcat -s AnyChatExample
```

#### iOS

```bash
cd packages/ios/Example
pod install
open AnyChatSDKExample.xcworkspace
# Run in Xcode (Cmd+R)
```

#### Web

```bash
cd packages/web/example
npm install
npm run dev
# Open http://localhost:5173
```

### Step 3: Test Connection

1. **Launch the app**
2. **Check connection status** (should show "Connected")
3. **Login** with test credentials:
   - Account: `test@example.com`
   - Password: `password123`
4. **Send a test message**
5. **Check server logs** for incoming requests

## Troubleshooting

### Connection Refused

**Symptom**: "Connection refused" or "Failed to connect"

**Solutions**:
1. Verify server is running: `curl http://localhost:8080/api/v1/health`
2. Check firewall allows port 8080:
   ```bash
   # Linux: Check firewall
   sudo ufw status
   sudo ufw allow 8080/tcp
   
   # macOS: Check firewall
   sudo /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate
   ```
3. Verify IP address is correct (not `127.0.0.1`)
4. Ensure server binds to `0.0.0.0`, not `127.0.0.1`

### Connection Timeout

**Symptom**: App hangs on "Connecting..."

**Solutions**:
1. Verify network connectivity:
   ```bash
   ping 192.168.2.100
   ```
2. Check if devices are on same network
3. Disable VPN if active
4. Try from same machine first (`127.0.0.1`)

### WebSocket Upgrade Failed

**Symptom**: "WebSocket connection failed" or "Upgrade failed"

**Solutions**:
1. Ensure server supports WebSocket protocol
2. Check server logs for WebSocket upgrade requests
3. Verify URL uses `ws://` (not `wss://` for local testing)
4. Test WebSocket directly:
   ```bash
   wscat -c ws://192.168.2.100:8080/api/v1/ws
   ```

### SSL/TLS Errors

**Symptom**: "SSL handshake failed" or "Certificate error"

**Solution**: For local development, use **unencrypted** connections:
- ❌ `wss://` and `https://` (requires SSL certificates)
- ✅ `ws://` and `http://` (plain text, fine for local testing)

### Mobile Device Can't Connect

**Symptom**: Works on PC but not on phone/tablet

**Solutions**:
1. Verify both devices on same WiFi network
2. Disable WiFi isolation/AP isolation on router
3. Check mobile device firewall
4. Use IP address, not `localhost` or hostname
5. Ensure server listens on `0.0.0.0`, not `127.0.0.1`

## Network Configuration

### Router Settings

If testing from multiple devices, ensure:

1. **WiFi Isolation OFF** (also called AP Isolation)
   - Go to router settings → WiFi → Advanced
   - Disable "Client Isolation" or "AP Isolation"

2. **Port Forwarding** (optional, for external access)
   - Forward external port 8080 → internal IP `192.168.2.100:8080`

### Firewall Rules

#### Linux (ufw)

```bash
# Allow port 8080
sudo ufw allow 8080/tcp
sudo ufw reload
```

#### macOS

```bash
# Allow incoming connections for your server
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /path/to/server
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock /path/to/server
```

#### Windows

```powershell
# Allow port 8080
netsh advfirewall firewall add rule name="AnyChat Server" dir=in action=allow protocol=TCP localport=8080
```

## Switching to Production

When ready for production testing:

1. **Update server URLs** to use HTTPS/WSS
2. **Use domain name** instead of IP
3. **Enable SSL/TLS** on server
4. **Update firewall rules** for production ports (usually 443 for HTTPS)

Example production config:
```dart
// Flutter
'wss://chat.yourcompany.com/api/v1/ws'
'https://chat.yourcompany.com/api/v1'
```

## Quick Reference

| Environment | WebSocket | HTTP | Use Case |
|-------------|-----------|------|----------|
| Local (same machine) | `ws://127.0.0.1:8080` | `http://127.0.0.1:8080` | Single machine testing |
| Local (LAN) | `ws://192.168.x.x:8080` | `http://192.168.x.x:8080` | Multi-device testing |
| Production | `wss://api.example.com` | `https://api.example.com` | Live deployment |

## Example Server Output

When everything is working, you should see:

```
[INFO] Server started on 0.0.0.0:8080
[INFO] WebSocket endpoint: ws://0.0.0.0:8080
[INFO] HTTP API endpoint: http://0.0.0.0:8080/api/v1
[INFO] Client connected from 192.168.2.101
[INFO] WebSocket upgraded for client: client-abc123
[INFO] User authenticated: test@example.com
[INFO] Message sent: conv-123 -> msg-456
```

## Support

If you encounter issues not covered here:
1. Check server logs for errors
2. Check client console/logcat for errors
3. Use network debugging tools (Wireshark, Charles Proxy)
4. Open an issue: https://github.com/yzhgit/anychat-sdk/issues
