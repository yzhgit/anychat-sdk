#pragma once

#include <functional>
#include <string>

namespace anychat {

struct AnyChatCallback {
    std::function<void()> on_success;
    std::function<void(int code, const std::string& error)> on_error;
};

template<typename T>
struct AnyChatValueCallback {
    std::function<void(const T& value)> on_success;
    std::function<void(int code, const std::string& error)> on_error;
};

} // namespace anychat
