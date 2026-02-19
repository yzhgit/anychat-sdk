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

- **HTTP**：libcurl（CURLM multi interface 实现异步请求）
- **WebSocket**：libwebsockets
- **JSON**：nlohmann/json
- **包管理**：CPM（CMake Package Manager）


## 开发规范

- 接口命名与后端 OpenAPI operationId 保持对应
- 错误码直接透传后端 `code` 字段，不做二次映射
- 分页参数统一：`page`（从 1 开始）、`pageSize`（默认 20）


## 相关链接

- 后端 API 文档：https://yzhgit.github.io/anychat-server
- 后端仓库：https://github.com/yzhgit/anychat-server
