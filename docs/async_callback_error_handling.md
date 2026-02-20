# 异步回调数据生命周期统一方案

## 问题描述

在 C API 与 Dart FFI 跨语言异步回调中，遇到了三个关键问题：

### 1. 错误字符串生命周期问题

C++ 回调函数中的 `const std::string& error` 参数是临时的：
- C++ lambda 执行完毕后，error 字符串立即被销毁
- Dart 的 `NativeCallable.listener` 需要通过消息传递跨 isolate 调用
- 当 Dart 回调真正执行时，C 字符串指针指向的内存可能已被释放

### 2. Token 结构生命周期问题

同样的问题也存在于返回的数据结构中：
- C++ 回调中的 `AnyChatAuthToken_C c_token` 是栈上局部变量
- lambda 返回后，token 结构立即被销毁
- Dart 异步回调访问 token 时读取到垃圾数据
- 错误信息：`Invalid argument(s): [-48, -31, 31, -6, -67, 124]`（无效的 UTF-8 字节）

### 3. UTF-8 解码错误

服务器返回的中文错误消息在 Dart FFI 边界传递时出现编码问题：
- 错误信息：`FormatException: Unexpected extension byte`
- 原因：错误字符串或 token 数据在异步传递过程中内存被覆盖或释放

## 统一解决方案

### 核心思想

**在 handle 中为每个子模块分配专用的错误缓冲区和数据缓冲区，确保所有异步回调数据的生命周期与 handle 相同。**

### 实现步骤

#### 1. 在主 handle 结构中添加缓冲区

```cpp
// core/src/c_api/handles_c.h
struct AnyChatClient_T {
    std::shared_ptr<anychat::AnyChatClient> impl;

    // 子模块 handles
    AnyChatAuthManager_T  auth_handle;
    AnyChatMessage_T      msg_handle;
    // ...

    // 为每个子模块分配专用错误缓冲区
    std::mutex        auth_error_mutex;
    std::string       auth_error_buffer;

    std::mutex        msg_error_mutex;
    std::string       msg_error_buffer;

    // ... 其他模块的错误缓冲区 ...

    // Token 数据缓冲区（用于认证回调）
    std::mutex            auth_token_mutex;
    AnyChatAuthToken_C    auth_token_buffer;
};
```

#### 2. 子模块 handle 包含父指针

```cpp
struct AnyChatAuthManager_T {
    anychat::AuthManager*  impl;
    AnyChatClient_T*       parent;  // 指向父 handle
};
```

#### 3. 创建辅助宏简化错误存储

```cpp
// core/src/c_api/utils_c.h
#define ANYCHAT_STORE_ERROR(handle, error_type, error_str) \
    [&]() -> const char* { \
        if ((error_str).empty() || !(handle)) return ""; \
        std::lock_guard<std::mutex> lock((handle)->error_type##_mutex); \
        (handle)->error_type##_buffer = (error_str); \
        return (handle)->error_type##_buffer.c_str(); \
    }()
```

#### 4. 在 C API 回调中使用宏和缓冲区

```cpp
// core/src/c_api/auth_c.cpp
int anychat_auth_register(...) {
    auto* parent = handle->parent;
    handle->impl->registerUser(
        phone_or_email, password, verify_code,
        device_type, nickname,
        [parent, userdata, callback](bool success,
                             const anychat::AuthToken& token,
                             const std::string& error)
        {
            if (!callback) return;
            if (success) {
                // Token 存储在父 handle 的缓冲区中
                AnyChatAuthToken_C* c_token_ptr = nullptr;
                if (parent) {
                    std::lock_guard<std::mutex> lock(parent->auth_token_mutex);
                    tokenToCStruct(token, &parent->auth_token_buffer);
                    c_token_ptr = &parent->auth_token_buffer;
                }
                callback(userdata, 1, c_token_ptr, "");
            } else {
                // 错误字符串存储在父 handle 的缓冲区中
                callback(userdata, 0, nullptr,
                         ANYCHAT_STORE_ERROR(parent, auth_error, error));
            }
        });
    return ANYCHAT_OK;
}
```

#### 5. Dart FFI 层添加异常捕获

```dart
// packages/flutter/lib/src/anychat_client.dart
void _authCallbackNative(
  Pointer<Void> userdata,
  int success,
  Pointer<AnyChatAuthToken_C> token,
  Pointer<Char> error,
) {
  final id = userdata.address;
  final completer = _getCallback(id) as Completer<AuthToken>?;
  if (completer == null) return;

  if (success != 0 && token != nullptr) {
    final dartToken = AuthToken(
      accessToken: _copyFixedStringStatic(token.ref.access_token, 512),
      refreshToken: _copyFixedStringStatic(token.ref.refresh_token, 512),
      expiresAtMs: token.ref.expires_at_ms,
    );
    completer.complete(dartToken);
  } else {
    String errorMsg = 'Unknown error';
    if (error != nullptr) {
      try {
        errorMsg = error.cast<Utf8>().toDartString();
      } catch (e) {
        // 捕获 UTF-8 解码异常
        errorMsg = 'Error (encoding issue): ${e.toString()}';
      }
    }
    completer.completeError(Exception(errorMsg));
  }
  _unregisterCallback(id);
}
```

## 方案优势

### 1. **线程安全**
- 每个缓冲区都有独立的互斥锁保护
- 避免不同子模块间的锁竞争

### 2. **生命周期保证**
- 错误字符串和数据结构都存储在 handle 中，生命周期与 handle 相同
- 即使 C++ 回调已经返回，所有数据仍然有效
- **解决了 token 结构被销毁的问题**

### 3. **统一模式**
- 所有子模块使用相同的错误和数据处理模式
- `ANYCHAT_STORE_ERROR` 宏确保一致性
- Token 等返回数据也遵循相同的缓冲区模式

### 4. **防御性编程**
- Dart 层 try-catch 捕获所有 UTF-8 解码异常
- C++ 层空检查防止空指针错误
- 缓冲区确保数据在异步传递期间不被销毁

### 5. **性能优化**
- 使用 lambda 表达式避免不必要的拷贝
- 只在需要时才加锁和存储
- 复用缓冲区减少内存分配

## 适用场景

这个方案适用于以下所有 C API 回调场景：

- ✅ **认证模块**: login, register, logout, refreshToken, changePassword
- ✅ **消息模块**: sendTextMessage, markMessageRead, getMessageHistory
- ✅ **会话模块**: getConversations, markConversationRead
- ✅ **好友模块**: getFriends, sendFriendRequest, handleFriendRequest
- ✅ **群组模块**: createGroup, joinGroup, quitGroup, getGroupMembers
- ✅ **客户端模块**: login, logout (client-level)

## 使用指南

### 添加新的异步回调函数

1. **确保父指针已设置**
   ```cpp
   auto* parent = handle->parent;
   ```

2. **在 lambda 中捕获父指针**
   ```cpp
   [parent, userdata, callback](bool success, const T& data, const std::string& error)
   ```

3. **使用宏存储错误**
   ```cpp
   callback(userdata, 0, nullptr,
            ANYCHAT_STORE_ERROR(parent, <module>_error, error));
   ```

4. **Dart 层添加 try-catch**
   ```dart
   try {
     errorMsg = error.cast<Utf8>().toDartString();
   } catch (e) {
     errorMsg = 'Error (encoding issue): ${e.toString()}';
   }
   ```

## 注意事项

1. **不要在回调外部使用错误缓冲区**：缓冲区可能被后续回调覆盖
2. **每个子模块使用独立缓冲区**：避免不同模块间的错误混淆
3. **始终检查 parent 指针**：宏内部已经处理，但手动代码需要注意
4. **Dart 层必须捕获异常**：防止未处理的 UTF-8 解码错误导致崩溃

## 测试验证

### 测试场景

1. **中文错误消息**：服务器返回中文错误时不应崩溃
2. **并发回调**：多个异步回调同时执行时数据不应混淆
3. **长时间延迟**：回调延迟执行时错误字符串和 token 数据仍然有效
4. **内存泄漏**：所有缓冲区正确释放，无内存泄漏
5. **Token 有效性**：注册/登录成功后 token 字符串完整且可解码

### 历史问题记录

#### 问题 1：UTF-8 解码错误（错误消息）
```
FormatException: Unexpected extension byte (at offset 2)
```
**原因**：错误字符串在异步回调前被销毁
**修复**：使用 handle 中的错误缓冲区

#### 问题 2：UTF-8 解码错误（Token 数据）
```
Invalid argument(s): [-48, -31, 31, -6, -67, 124]
```
**原因**：Token 结构在栈上，lambda 返回后被销毁
**修复**：使用 handle 中的 token 缓冲区

### 测试方法

```bash
# 编译 C++ 核心库
cd /home/mosee/projects/anychat-sdk
cmake --build build --target anychat_c

# 清理并运行 Flutter example
cd packages/flutter/example
flutter clean
flutter run

# 测试注册功能（验证中文错误消息和 token 处理）
# 1. 使用正确的验证码注册新用户 -> 应该成功并返回有效 token
# 2. 使用错误的验证码注册 -> 应该返回中文错误消息而不崩溃
```

## 相关文件

- `core/src/c_api/handles_c.h` - Handle 结构定义
- `core/src/c_api/utils_c.h` - ANYCHAT_STORE_ERROR 宏定义
- `core/src/c_api/auth_c.cpp` - 认证模块实现
- `core/src/c_api/client_c.cpp` - 客户端模块实现
- `packages/flutter/lib/src/anychat_client.dart` - Dart FFI 回调处理

## 版本历史

- **2025-02-20**: 初始版本，统一异步回调错误处理方案
