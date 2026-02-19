#include <gtest/gtest.h>
#include "anychat/types.h"

TEST(MessageTest, DefaultValues) {
    anychat::Message msg;
    EXPECT_TRUE(msg.message_id.empty());
    EXPECT_FALSE(msg.is_read);
    EXPECT_EQ(msg.timestamp_ms, 0);
}

TEST(UserInfoTest, Fields) {
    anychat::UserInfo user;
    user.user_id  = "u-001";
    user.username = "alice";
    EXPECT_EQ(user.user_id, "u-001");
    EXPECT_EQ(user.username, "alice");
}
