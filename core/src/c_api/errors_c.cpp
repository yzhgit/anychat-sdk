#include "anychat_c/errors_c.h"
#include <string>

/* Thread-local storage for the last error message on each thread. */
static thread_local std::string g_last_error;

void anychat_set_last_error(const std::string& error) {
    g_last_error = error;
}

void anychat_clear_last_error() {
    g_last_error.clear();
}

extern "C" {

const char* anychat_get_last_error(void) {
    return g_last_error.c_str();
}

} // extern "C"
