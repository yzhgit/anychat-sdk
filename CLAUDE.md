# AnyChat SDK

## 项目说明

本项目是 AnyChat IM 系统的客户端 SDK，封装后端接口，提供统一的接入层。
目标平台：Android / iOS / macOS / Linux / Windows / Flutter / Web


## SDK 架构

### 三层架构

```
┌─────────────────────────────────────────┐
│  平台 SDK (Dart/Java/Swift/JS)          │
│  - 高层次、符合平台习惯的 API           │
│  - Future/Promise 异步模式              │
│  - 响应式 Stream/Observable             │
├─────────────────────────────────────────┤
│  平台绑定层                             │
│  - JNI (Android)                        │
│  - Objective-C/Swift 桥接 (iOS/macOS)   │
│  - Dart FFI (Flutter)                   │
│  - Emscripten (Web)                     │
├─────────────────────────────────────────┤
│  C API 层 (anychat_c)                   │
│  - 稳定的 C ABI（跨编译器兼容）         │
│  - Opaque handles + C 回调              │
│  - 错误码 + TLS 错误消息                │
├─────────────────────────────────────────┤
│  C++ 核心库 (anychat_core)              │
│  - WebSocket 客户端 (libwebsockets)     │
│  - HTTP 客户端 (libcurl)                │
│  - SQLite 数据库（本地缓存）            │
│  - 业务逻辑 & 状态管理                  │
└─────────────────────────────────────────┘
```

### 为什么需要 C API 层？

C++ ABI 在不同编译器（MSVC、GCC、Clang）之间不兼容，即使同一编译器的不同版本也可能不兼容。这导致：

- 用 MSVC 编译的 DLL 无法被 GCC 编译的程序调用
- STL 类型（`std::string`、`std::vector`）的内存布局不同
- 虚函数表布局、异常处理、name mangling 机制不同

**解决方案**：使用稳定的 C API（参考 SQLite、OpenSSL、FFmpeg 等业界标准）

- ✅ C ABI 跨编译器标准化且稳定
- ✅ 简化语言绑定（C 类型直接映射到 FFI）
- ✅ 无需 SWIG（每个平台使用原生绑定工具）


## 第三方依赖

所有第三方库通过 **Git Submodule** 方式集成（SQLite3 除外），位于 `thirdparty/` 目录。
首次 clone 或新成员拉取代码后，需执行：

```bash
git submodule update --init --recursive
```

| 库 | 路径 | 用途 | CMake 目标 |
|---|---|---|---|
| **curl** | `thirdparty/curl` | HTTP 异步请求（CURLM multi interface） | `CURL::libcurl` |
| **libwebsockets** | `thirdparty/libwebsockets` | WebSocket 客户端 | `websockets` |
| **nlohmann-json** | `thirdparty/nlohmann-json` | JSON 序列化/反序列化（仅头文件） | `nlohmann_json::nlohmann_json` |
| **googletest** | `thirdparty/googletest` | 单元测试框架 | `GTest::gtest_main` |
| **sqlite3** | `thirdparty/sqlite3` | SQLite3 amalgamation（sqlite3.c/h） | `SQLite::SQLite3` |

> **SQLite3** 使用 amalgamation 单文件形式（`sqlite3.c` + `sqlite3.h`）直接放置在
> `thirdparty/sqlite3/` 目录，编译为静态库目标 `sqlite3`，**不依赖系统 SQLite3 安装**。

### 数据库层封装

数据库层（`core/src/db/database.cpp`）使用 SQLite C API 提供以下功能：
- **参数化 SQL**：通过 `execSync()`/`querySync()` 和 `exec()`/`query()` 异步接口执行 SQL
- **事务支持**：`transactionSync()` 提供原子事务，自动 BEGIN/COMMIT/ROLLBACK
- **Worker 线程**：所有数据库操作在单独的 worker 线程执行，避免阻塞主线程
- **WAL 模式**：启用 Write-Ahead Logging 模式，提升并发读性能


## C API 使用

C API 位于 `core/include/anychat_c/`，提供稳定的跨编译器接口：

```c
#include <anychat_c/anychat_c.h>

// 创建客户端
AnyChatClientConfig_C config = {
    .gateway_url = "wss://api.anychat.io",
    .api_base_url = "https://api.anychat.io/api/v1",
    .device_id = "my-device-001",
    .db_path = "./anychat.db",
};
AnyChatClientHandle client = anychat_client_create(&config);

// 连接
anychat_client_connect(client);

// 登录
AnyChatAuthHandle auth = anychat_client_get_auth(client);
anychat_auth_login(auth, "user@example.com", "password", "desktop", NULL, callback);

// 清理
anychat_client_disconnect(client);
anychat_client_destroy(client);
```

详见：
- `docs/c_api_guide.md` — C API 使用指南
- `examples/c_example/main.c` — 完整示例


## 平台绑定

### Flutter (Dart FFI) ✅

使用 `ffigen` 自动生成 FFI 绑定：

```bash
cd bindings/flutter
dart run ffigen --config ffigen.yaml  # 生成 lib/src/anychat_ffi_bindings.dart
flutter pub get
cd example && flutter run
```

详见：`bindings/flutter/README.md`

### Android (JNI) 🚧

```bash
cd bindings/android
./gradlew assembleRelease
```

*(开发中)*

### iOS/macOS (Swift) 🚧

```bash
cd bindings/ios
pod install
open AnyChatSDK.xcworkspace
```

*(开发中)*

### Web (Emscripten) 🚧

```bash
cd bindings/web
emcmake cmake -B build
cmake --build build
```

*(开发中)*


## 开发规范

- 接口命名与后端 OpenAPI operationId 保持对应
- 错误码直接透传后端 `code` 字段，不做二次映射
- 分页参数统一：`page`（从 1 开始）、`pageSize`（默认 20）
- C API 使用 UTF-8 编码的字符串
- 所有回调使用 `void* userdata` 传递上下文


## 构建步骤

### 1. 初始化子模块

```bash
git submodule update --init --recursive
```

### 2. 构建 C++ 核心 + C API

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build  # 运行单元测试
```

生成：
- `anychat_core` (C++ 静态库)
- `anychat_c` (C 静态库)
- `anychat_c_shared` (C 动态库，可选)

### 3. 构建平台 SDK

参见各平台 README：
- `bindings/flutter/README.md`
- `bindings/android/README.md` *(TBD)*
- `bindings/ios/README.md` *(TBD)*


## 测试

```bash
# C++ 单元测试
cd build && ctest

# C API 示例
./build/bin/c_example

# 内存泄漏检测
valgrind --leak-check=full ./build/bin/c_example

# Flutter 测试
cd bindings/flutter && flutter test
```


## 相关链接

- **后端 API 文档**：https://yzhgit.github.io/anychat-server
- **后端仓库**：https://github.com/yzhgit/anychat-server
- **C API 指南**：`docs/c_api_guide.md`
- **Flutter SDK 指南**：`bindings/flutter/README.md`
