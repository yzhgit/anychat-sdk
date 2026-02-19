/*
 * AnyChat SDK — C interface usage example
 *
 * Build this file together with anychat_c (static) and anychat_core.
 * See docs/c_api_guide.md for detailed instructions.
 *
 * Compile example (Linux):
 *   gcc -std=c11 main.c -I../../core/include \
 *       -L../../build/bin -lanychat_c -lanychat_core \
 *       -lstdc++ -lpthread -o c_example
 */

#include <anychat_c/anychat_c.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Callbacks ---- */

static void on_connection_state(void* userdata, int state) {
    const char* names[] = {"Disconnected", "Connecting", "Connected", "Reconnecting"};
    const char* name = (state >= 0 && state <= 3) ? names[state] : "Unknown";
    printf("[conn] state -> %s\n", name);
}

static void on_login(void* userdata,
                     int success,
                     const AnyChatAuthToken_C* token,
                     const char* error)
{
    if (success) {
        printf("[auth] login OK  access_token=%.40s...\n", token->access_token);
    } else {
        printf("[auth] login FAIL: %s\n", error);
    }
}

static void on_logout(void* userdata, int success, const char* error) {
    if (success) printf("[auth] logout OK\n");
    else         printf("[auth] logout FAIL: %s\n", error);
}

static void on_message_received(void* userdata, const AnyChatMessage_C* msg) {
    printf("[msg] received  conv=%s  sender=%s  content=%s\n",
           msg->conv_id, msg->sender_id, msg->content ? msg->content : "(null)");
}

static void on_send_message(void* userdata, int success, const char* error) {
    if (success) printf("[msg] sent OK\n");
    else         printf("[msg] send FAIL: %s\n", error);
}

static void on_get_history(void* userdata,
                           const AnyChatMessageList_C* list,
                           const char* error)
{
    if (error) { printf("[msg] history FAIL: %s\n", error); return; }
    printf("[msg] history count=%d\n", list->count);
    for (int i = 0; i < list->count; ++i) {
        printf("  [%d] %s: %s\n",
               i, list->items[i].sender_id,
               list->items[i].content ? list->items[i].content : "");
    }
}

/* ---- Main ---- */

int main(void) {
    /* 1. Configure the client */
    AnyChatClientConfig_C config;
    memset(&config, 0, sizeof(config));
    config.gateway_url            = "wss://api.anychat.io";
    config.api_base_url           = "https://api.anychat.io/api/v1";
    config.device_id              = "example-device-001";
    config.db_path                = "./anychat_example.db";
    config.connect_timeout_ms     = 10000;
    config.max_reconnect_attempts = 5;
    config.auto_reconnect         = 1;

    /* 2. Create the client */
    AnyChatClientHandle client = anychat_client_create(&config);
    if (!client) {
        fprintf(stderr, "Failed to create client: %s\n", anychat_get_last_error());
        return 1;
    }

    /* 3. Register connection state callback */
    anychat_client_set_connection_callback(client, NULL, on_connection_state);

    /* 4. Connect */
    anychat_client_connect(client);

    /* 5. Get sub-module handles */
    AnyChatAuthHandle    auth    = anychat_client_get_auth(client);
    AnyChatMessageHandle message = anychat_client_get_message(client);

    /* 6. Register incoming message handler */
    anychat_message_set_received_callback(message, NULL, on_message_received);

    /* 7. Login */
    int ret = anychat_auth_login(auth,
                                  "user@example.com",
                                  "s3cr3tpassw0rd",
                                  "web",
                                  NULL,
                                  on_login);
    if (ret != ANYCHAT_OK) {
        fprintf(stderr, "Login request failed: %s\n", anychat_get_last_error());
    }

    /* 8. Send a text message (assumes login has completed and conv_id is known) */
    ret = anychat_message_send_text(message,
                                     "conv-abc-123",
                                     "Hello from the C API!",
                                     NULL,
                                     on_send_message);
    if (ret != ANYCHAT_OK) {
        fprintf(stderr, "Send message failed: %s\n", anychat_get_last_error());
    }

    /* 9. Fetch message history */
    ret = anychat_message_get_history(message,
                                       "conv-abc-123",
                                       0,   /* before_timestamp_ms = 0 → most recent */
                                       20,  /* limit */
                                       NULL,
                                       on_get_history);
    if (ret != ANYCHAT_OK) {
        fprintf(stderr, "Get history failed: %s\n", anychat_get_last_error());
    }

    /* 10. Check login state */
    if (anychat_auth_is_logged_in(auth)) {
        AnyChatAuthToken_C token;
        if (anychat_auth_get_current_token(auth, &token) == ANYCHAT_OK) {
            printf("[auth] current token expires_at_ms=%" PRId64 "\n",
                   token.expires_at_ms);
        }
    }

    /* 11. Logout */
    anychat_auth_logout(auth, NULL, on_logout);

    /* 12. Clean up */
    anychat_client_disconnect(client);
    anychat_client_destroy(client);

    printf("Example done.\n");
    return 0;
}
