# AnyChat SDK

Multi-platform client SDK for the [AnyChat](https://github.com/yzhgit/anychat-server) instant messaging system.

The repository provides:
- A native core library implemented in C++
- A public C ABI consumed by platform bindings
- Platform packages for Android, iOS, Flutter, and Web

## Repository Structure

```text
anychat-sdk/
|-- core/
|   |-- include/anychat/        # Public SDK headers (single entry: anychat.h)
|   |-- src/                    # Core implementation (managers, network, DB, C API wrappers)
|   `-- tests/                  # GoogleTest test suites
|-- packages/
|   |-- android/                # Android binding/package
|   |-- ios/                    # iOS binding/package
|   |-- flutter/                # Flutter binding/package
|   `-- web/                    # Web binding/package
|-- thirdparty/                 # Submodule / vendored dependencies
|-- docs/                       # Documentation
|-- scripts/                    # Build and automation scripts
|-- CMakeLists.txt
`-- CMakePresets.json
```

## Public Headers

Use this as the single include entry point:

```c
#include <anychat/anychat.h>
```

`anychat/anychat.h` includes module headers such as:
- `client.h`
- `auth.h`
- `message.h`
- `conversation.h`
- `friend.h`
- `group.h`
- `file.h`
- `user.h`
- `call.h`
- `version.h`
- `types.h`
- `errors.h`

## C++ Compiler and Language Requirements

- CMake: `3.20+`
- Build standard: `C++23` (configured globally by the root `CMakeLists.txt`)
- Compiler: use a compiler toolchain that supports C++23 (GCC / Clang / MSVC)
- Coding policy: first-party business code in `core/` and `packages/` should stay within C++17-era features unless explicitly approved

## Build Commands

Initialize dependencies:

```bash
git submodule update --init --recursive
```

Configure and build:

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build -j
```

Run C++ tests:

```bash
ctest --test-dir build --output-on-failure
```

Preset-based build (optional):

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

## Minimal C API Usage

```c
#include <anychat/anychat.h>
#include <stdio.h>

static void on_login_ok(void* userdata, const AnyChatAuthToken_C* token) {
    (void)userdata;
    printf("login success, access token: %s\n", token ? token->access_token : "");
}

static void on_login_error(void* userdata, int code, const char* error) {
    (void)userdata;
    printf("login failed, code=%d, error=%s\n", code, error ? error : "");
}

int main(void) {
    AnyChatClientConfig_C cfg = {0};
    cfg.gateway_url = "wss://api.anychat.io";
    cfg.api_base_url = "https://api.anychat.io/api/v1";
    cfg.device_id = "desktop-001";
    cfg.db_path = "./anychat.db";
    cfg.connect_timeout_ms = 10000;
    cfg.max_reconnect_attempts = 5;
    cfg.auto_reconnect = 1;

    AnyChatClientHandle client = anychat_client_create(&cfg);
    if (!client) return 1;

    AnyChatAuthTokenCallback_C cb = {0};
    cb.on_success = on_login_ok;
    cb.on_error = on_login_error;

    int rc = anychat_client_login(
        client,
        "user@example.com",
        "password",
        "web",
        "1.0.0",
        &cb
    );
    if (rc != ANYCHAT_OK) {
        anychat_client_destroy(client);
        return 2;
    }

    /* Run your app loop here. */

    anychat_client_logout(client, NULL);
    anychat_client_destroy(client);
    return 0;
}
```

## Package Docs

- [packages/README.md](packages/README.md)
- [packages/android/README.md](packages/android/README.md)
- [packages/ios/README.md](packages/ios/README.md)
- [packages/flutter/README.md](packages/flutter/README.md)
- [packages/web/README.md](packages/web/README.md)

## License

MIT. See [LICENSE](LICENSE).
