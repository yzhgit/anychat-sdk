#pragma once

/* Platform visibility macros */
#ifdef _WIN32
  #ifdef ANYCHAT_C_EXPORTS
    #define ANYCHAT_C_API __declspec(dllexport)
  #else
    #define ANYCHAT_C_API __declspec(dllimport)
  #endif
#else
  #define ANYCHAT_C_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes (0 = success) */
#define ANYCHAT_OK                   0
#define ANYCHAT_ERROR_INVALID_PARAM  1
#define ANYCHAT_ERROR_AUTH           2
#define ANYCHAT_ERROR_NETWORK        3
#define ANYCHAT_ERROR_TIMEOUT        4
#define ANYCHAT_ERROR_NOT_FOUND      5
#define ANYCHAT_ERROR_ALREADY_EXISTS 6
#define ANYCHAT_ERROR_INTERNAL       7
#define ANYCHAT_ERROR_NOT_LOGGED_IN  8
#define ANYCHAT_ERROR_TOKEN_EXPIRED  9

/* Returns the last error message for the calling thread.
 * The returned pointer is valid until the next SDK call on the same thread.
 * Never NULL — returns empty string when there is no error. */
ANYCHAT_C_API const char* anychat_get_last_error(void);

#ifdef __cplusplus
}
#endif
