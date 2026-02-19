# AnyChat SDK

## 项目说明

本项目是 AnyChat IM 系统的客户端 SDK，封装后端接口，提供统一的接入层。
目标平台：[Web / iOS / Android / 桌面端]


## SDK 架构约定

- 网络层统一封装在 `src/network/` 或 `lib/network/`
- 所有请求自动附加 Authorization header
- WebSocket 心跳间隔：30s
- 断线自动重连，最大重试：5 次，指数退避


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


## 开发规范

- 接口命名与后端 OpenAPI operationId 保持对应
- 错误码直接透传后端 `code` 字段，不做二次映射
- 分页参数统一：`page`（从 1 开始）、`pageSize`（默认 20）


## 相关链接

- 后端 API 文档：https://yzhgit.github.io/anychat-server
- 后端仓库：https://github.com/yzhgit/anychat-server
