#pragma once

/* Internal helpers shared across c_api implementation files.
 * Not part of the public API. */

#include <cstddef>
#include <string>
#include <mutex>

/* Duplicate a C string with malloc. Returns nullptr if s is nullptr. */
char* anychat_strdup(const char* s);

/* Safe string copy: always NUL-terminates, truncates if necessary. */
void anychat_strlcpy(char* dst, const char* src, size_t dst_size);

/* Set / clear the thread-local last-error message. */
void anychat_set_last_error(const std::string& error);
void anychat_clear_last_error();

/* Helper macro to safely store error string in handle's buffer for async callbacks.
 * Usage: ANYCHAT_STORE_ERROR(parent_handle, auth_error, error_string)
 * Returns: const char* pointing to the stored error (or "" if error is empty)
 */
#define ANYCHAT_STORE_ERROR(handle, error_type, error_str) \
    [&]() -> const char* { \
        if ((error_str).empty() || !(handle)) return ""; \
        std::lock_guard<std::mutex> lock((handle)->error_type##_mutex); \
        (handle)->error_type##_buffer = (error_str); \
        return (handle)->error_type##_buffer.c_str(); \
    }()
