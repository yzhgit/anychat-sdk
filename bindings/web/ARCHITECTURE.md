# AnyChat Web Bindings Architecture

## Overview

The AnyChat Web bindings provide a complete WebAssembly-based SDK for building instant messaging applications in web browsers. The architecture consists of three main layers:

```
┌─────────────────────────────────────────────────────────────┐
│                     TypeScript Layer                         │
│  (index.ts, AnyChatClient.ts, types.ts, EventEmitter.ts)   │
│         Promise-based API + Event-driven interface          │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ JavaScript/TypeScript API
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Emscripten Embind Layer                   │
│                  (embind_wrapper.cpp)                        │
│       C++ ↔ JavaScript bindings with automatic type         │
│                      conversion                              │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ C API calls
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                        C API Layer                           │
│              (core/include/anychat_c/*.h)                    │
│         Stable C interface to C++ core library              │
└─────────────────────────────────────────────────────────────┘
                            ▲
                            │ C++ implementation
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      C++ Core Library                        │
│                  (anychat_core library)                      │
│   Business logic, networking, database, state management    │
└─────────────────────────────────────────────────────────────┘
```

## Layer Details

### 1. TypeScript Layer

**Purpose**: Provide a developer-friendly, Promise-based API with full TypeScript support.

**Key Files**:
- `src/index.ts` - Main entry point and module initialization
- `src/AnyChatClient.ts` - Main client class with Promise-based methods
- `src/types.ts` - TypeScript type definitions and enums
- `src/EventEmitter.ts` - Event emitter implementation for real-time events

**Features**:
- Promise-based async operations (login, sendMessage, etc.)
- EventEmitter pattern for real-time events (messageReceived, connectionStateChanged, etc.)
- Full TypeScript type safety
- Error handling with custom AnyChatError class
- Modern ES6+ syntax

**Example**:
```typescript
const client = await createAnyChatClient(config);
client.on('messageReceived', (msg) => console.log(msg));
await client.login('user@example.com', 'password');
await client.sendTextMessage('conv-id', 'Hello!');
```

### 2. Emscripten Embind Layer

**Purpose**: Bridge between JavaScript and C++ using Emscripten's embind system.

**Key File**: `src/embind_wrapper.cpp`

**Responsibilities**:
1. **Type Conversion**: Convert C structs to JavaScript objects
   - `authTokenToJS()` - Convert C auth token to JS object
   - `messageToJS()` - Convert C message struct to JS object
   - `conversationToJS()` - Convert C conversation to JS object
   - etc.

2. **Callback Management**: Store and invoke JavaScript callbacks from C
   - Global `CallbackStore` maintains callback references
   - Callback wrappers translate C callbacks to JavaScript calls
   - Automatic cleanup of one-time callbacks

3. **Wrapper Class**: `AnyChatClientWrapper` class exposed to JavaScript
   - Wraps C API calls
   - Manages C handles (client, auth, message, etc.)
   - Provides JavaScript-friendly method signatures

4. **Embind Registration**: Register C++ classes and functions for JavaScript access
   ```cpp
   EMSCRIPTEN_BINDINGS(anychat) {
       class_<AnyChatClientWrapper>("AnyChatClientWrapper")
           .constructor<val>()
           .function("login", &AnyChatClientWrapper::login)
           // ... more functions
   }
   ```

### 3. C API Layer

**Purpose**: Stable C interface to the C++ core library.

**Key Files**: All headers in `core/include/anychat_c/`
- `anychat_c.h` - Main header (includes all others)
- `client_c.h` - Client lifecycle and connection management
- `auth_c.h` - Authentication operations
- `message_c.h` - Message operations
- `conversation_c.h` - Conversation management
- `friend_c.h` - Friend operations
- `group_c.h` - Group operations
- etc.

**Design Principles**:
- Pure C interface (no C++ features)
- Opaque handle types (e.g., `AnyChatClientHandle`)
- Plain-old-data (POD) structs for data exchange
- Explicit memory management with `anychat_free_*()` functions
- Callback-based async operations

### 4. C++ Core Library

**Purpose**: Implement all business logic, networking, and data persistence.

**Key Components**:
- `Client` - Main client class
- `AuthManager` - Authentication and token management
- `MessageManager` - Message sending/receiving
- `ConversationManager` - Conversation state
- `ConnectionManager` - WebSocket connection handling
- `Database` - SQLite-based local storage
- `HttpClient` - REST API communication
- `WebSocketClient` - Real-time communication

## Build Process

### 1. CMake Configuration

`CMakeLists.txt` configures the Emscripten build:

```cmake
# Build embind wrapper
add_executable(anychat_wasm src/embind_wrapper.cpp)
target_link_libraries(anychat_wasm PRIVATE anychat_c)

# Emscripten linker flags
target_link_options(anychat_wasm PRIVATE
    "SHELL:--bind"                    # Enable embind
    "SHELL:-s MODULARIZE=1"           # Export as ES6 module
    "SHELL:-s EXPORT_NAME='createAnyChatModule'"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"  # Dynamic heap
    "SHELL:--post-js ..."             # Post-initialization JS
)
```

### 2. Compilation Flow

```
embind_wrapper.cpp
      ↓ (emcc with --bind)
anychat.js + anychat.wasm
      ↓ (loaded by TypeScript)
WASM Module Instance
      ↓ (wrapped by AnyChatClient)
Final SDK
```

### 3. Build Command

```bash
emcmake cmake .. -DBUILD_WEB_BINDING=ON
emmake make -j$(nproc)
```

Output:
- `anychat.js` - JavaScript loader with embind bindings
- `anychat.wasm` - Compiled WebAssembly binary

### 4. TypeScript Build

```bash
tsc  # Compiles src/*.ts to dist/*.js with .d.ts definitions
```

## Memory Management

### JavaScript → C

**Strings**: Automatically copied by embind, no manual management needed.

**Arrays**: Converted using embind's `val` type, automatic lifetime management.

### C → JavaScript

**Strings**: C API allocates, JavaScript must call `anychat_free_string()` (handled automatically by embind wrapper).

**Structs**: Converted to JavaScript objects in embind wrapper, memory freed after conversion.

**Lists**: C API allocates arrays, embind wrapper converts to JS arrays and frees C memory.

### WASM Heap

Emscripten manages the WASM heap automatically with `-s ALLOW_MEMORY_GROWTH=1`, which allows dynamic heap expansion.

## Threading Model

### C++ Core
- Main thread for WebSocket I/O
- Worker thread for database operations
- Callbacks fire on internal SDK threads

### Emscripten
- Single-threaded by default (uses Web Workers for some operations)
- All JavaScript callbacks invoked on main thread
- Async operations use JavaScript Promises

### Best Practices
- Don't block in event handlers
- Use `async/await` for all SDK operations
- Debounce frequent operations (like mark-as-read)

## Data Flow Examples

### Sending a Message

```
TypeScript:
  client.sendTextMessage('conv-id', 'Hello')
         ↓
embind_wrapper.cpp:
  AnyChatClientWrapper::sendTextMessage()
    - Store JS callback in CallbackStore
    - Call anychat_message_send_text()
         ↓
C API (message_c.cpp):
  anychat_message_send_text()
    - Validate parameters
    - Call MessageManager::sendText()
         ↓
C++ Core (message_manager.cpp):
  MessageManager::sendText()
    - Insert to local DB
    - Queue for network send
    - Invoke C callback when sent
         ↓
embind_wrapper.cpp:
  messageCallbackWrapper()
    - Lookup JS callback in CallbackStore
    - Invoke JS callback(error)
    - Remove from CallbackStore
         ↓
TypeScript:
  Promise resolves or rejects
```

### Receiving a Message

```
C++ Core:
  WebSocket receives message data
         ↓
  MessageManager::handleIncomingMessage()
    - Parse message
    - Store in database
    - Invoke C callback
         ↓
C API:
  AnyChatMessageReceivedCallback fires
         ↓
embind_wrapper.cpp:
  messageReceivedCallbackWrapper()
    - Convert C message struct to JS object
    - Invoke stored JS callback
         ↓
TypeScript:
  client.emit('messageReceived', message)
         ↓
  Application event handler receives message
```

## Testing

### Unit Tests (C++ Core)
- Located in `core/tests/`
- Uses Google Test framework
- Tests C++ core logic in isolation

### Integration Tests
- Test C API layer
- Verify callback mechanisms
- Test memory management

### Web Tests
- Manual testing using example app
- Browser compatibility testing
- Performance profiling

## Performance Considerations

### Bundle Size
- WASM binary: ~500KB (gzipped: ~150KB)
- JavaScript glue: ~50KB (gzipped: ~15KB)
- TypeScript layer: ~30KB (gzipped: ~8KB)

### Optimization Tips
1. **Lazy Loading**: Load WASM module only when needed
2. **Code Splitting**: Separate SDK from application code
3. **Compression**: Enable gzip/brotli on server
4. **Caching**: Set proper cache headers for WASM file
5. **In-Memory DB**: Use `:memory:` for dbPath to avoid IndexedDB overhead

### Runtime Performance
- WASM execution is near-native speed
- Database operations run in worker thread
- Network I/O is non-blocking
- Event callbacks have minimal overhead

## Security

### Considerations
1. **XSS Protection**: Sanitize message content before rendering
2. **HTTPS/WSS**: Always use secure protocols in production
3. **Token Storage**: Store auth tokens securely (HttpOnly cookies recommended)
4. **Input Validation**: Validate all user input
5. **CORS**: Configure proper CORS headers for WASM/API access

### Best Practices
- Never expose credentials in client code
- Implement rate limiting on server side
- Use Content Security Policy (CSP)
- Regularly update dependencies
- Monitor for security vulnerabilities

## Debugging

### Browser DevTools
- Check Network tab for WebSocket/HTTP traffic
- Use Console for error messages
- Profile WASM performance in Performance tab
- Inspect memory usage in Memory tab

### WASM Debugging
- Build with `-O0 -g` for debug symbols
- Use browser WASM debugger
- Enable Emscripten logging: `-s ASSERTIONS=1`

### Logging
```typescript
// Enable SDK debug logging
const client = await createAnyChatClient({
  ...config,
  logLevel: 'debug', // Add this option if implemented
});
```

## Future Enhancements

### Potential Improvements
1. **Web Workers**: Offload WASM to worker thread
2. **Streaming**: Implement streaming for large file uploads
3. **Service Worker**: Add offline support
4. **WebRTC**: Native browser integration for calls
5. **IndexedDB**: Optional persistent storage for web
6. **Compression**: Message compression for bandwidth savings

### API Additions
- Typing indicators
- Read receipts
- Message reactions
- Voice messages
- File attachments
- Search functionality

## Resources

- [Emscripten Documentation](https://emscripten.org/)
- [Embind Documentation](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html)
- [WebAssembly Specification](https://webassembly.github.io/spec/)
- [TypeScript Handbook](https://www.typescriptlang.org/docs/handbook/)
