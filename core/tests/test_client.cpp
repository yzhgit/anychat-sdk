#include <gtest/gtest.h>
#include "anychat/client.h"

TEST(AnyChatClientTest, CreateThrowsUntilImplemented) {
    anychat::ClientConfig cfg;
    cfg.gateway_url  = "wss://localhost:8080";
    cfg.api_base_url = "https://localhost:8080";
    EXPECT_THROW(anychat::AnyChatClient::create(cfg), std::runtime_error);
}
