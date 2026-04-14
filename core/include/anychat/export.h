#pragma once

/*
 * Export macro for C API functions.
 *
 * - When building as a shared library (DLL), ANYCHAT_C_EXPORTS is defined,
 *   and functions are marked with __declspec(dllexport).
 * - When using the shared library, functions are marked with __declspec(dllimport).
 * - When building or using as a static library, no special attributes are needed.
 */

#if defined(_WIN32) || defined(_WIN64)
    #ifdef ANYCHAT_C_SHARED
        #ifdef ANYCHAT_C_EXPORTS
            #define ANYCHAT_C_API __declspec(dllexport)
        #else
            #define ANYCHAT_C_API __declspec(dllimport)
        #endif
    #else
        // Static library: no special attributes needed
        #define ANYCHAT_C_API
    #endif
#else
    // Non-Windows platforms
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define ANYCHAT_C_API __attribute__((visibility("default")))
    #else
        #define ANYCHAT_C_API
    #endif
#endif
