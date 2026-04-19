#include "client_impl.h"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(AnyChatClientTest, ConstructorValidatesRequiredFields) {
    anychat::ClientConfig cfg;
    cfg.gateway_url = "wss://localhost:8080";
    cfg.api_base_url = "https://localhost:8080";
    EXPECT_THROW(anychat::AnyChatClient client(cfg), std::invalid_argument);
}
