#include "jni_helpers.h"

using namespace anychat::jni;

extern JavaVM* g_jvm;

static void authTokenSuccess(void* userdata, const AnyChatAuthToken_C* token) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onAuthResult",
        "(ZLcom/anychat/sdk/models/AuthToken;Ljava/lang/String;)V");

    if (mid) {
        jobject tokenObj = nullptr;
        if (token) {
            tokenObj = convertAuthToken(env, *token);
        }
        env->CallVoidMethod(ctx->callback, mid, JNI_TRUE, tokenObj, nullptr);

        if (tokenObj) env->DeleteLocalRef(tokenObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx; // Clean up callback context
}

static void authTokenError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onAuthResult",
        "(ZLcom/anychat/sdk/models/AuthToken;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, JNI_FALSE, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void verificationCodeSuccess(void* userdata, const AnyChatVerificationCodeResult_C* result) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(
        cls,
        "onVerificationCodeResult",
        "(ZLcom/anychat/sdk/models/VerificationCodeResult;Ljava/lang/String;)V"
    );

    if (mid) {
        jobject resultObj = nullptr;
        if (result) {
            resultObj = convertVerificationCodeResult(env, *result);
        }
        env->CallVoidMethod(ctx->callback, mid, JNI_TRUE, resultObj, nullptr);

        if (resultObj) env->DeleteLocalRef(resultObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void verificationCodeError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(
        cls,
        "onVerificationCodeResult",
        "(ZLcom/anychat/sdk/models/VerificationCodeResult;Ljava/lang/String;)V"
    );

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, JNI_FALSE, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void resultSuccess(void* userdata) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onResult", "(ZLjava/lang/String;)V");

    if (mid) {
        env->CallVoidMethod(ctx->callback, mid, JNI_TRUE, nullptr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void resultError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onResult", "(ZLjava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, JNI_FALSE, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void authDeviceListSuccess(void* userdata, const AnyChatAuthDeviceList_C* list) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onAuthDeviceList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject devicesObj = nullptr;
        if (list) {
            devicesObj = convertAuthDeviceList(env, list);
        }
        env->CallVoidMethod(ctx->callback, mid, devicesObj, nullptr);

        if (devicesObj) env->DeleteLocalRef(devicesObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void authDeviceListError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onAuthDeviceList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static AnyChatAuthTokenCallback_C makeAuthTokenCallback(CallbackContext* ctx) {
    AnyChatAuthTokenCallback_C callback{};
    callback.struct_size = sizeof(callback);
    callback.userdata = ctx;
    callback.on_success = authTokenSuccess;
    callback.on_error = authTokenError;
    return callback;
}

static AnyChatVerificationCodeCallback_C makeVerificationCodeCallback(CallbackContext* ctx) {
    AnyChatVerificationCodeCallback_C callback{};
    callback.struct_size = sizeof(callback);
    callback.userdata = ctx;
    callback.on_success = verificationCodeSuccess;
    callback.on_error = verificationCodeError;
    return callback;
}

static AnyChatAuthResultCallback_C makeResultCallback(CallbackContext* ctx) {
    AnyChatAuthResultCallback_C callback{};
    callback.struct_size = sizeof(callback);
    callback.userdata = ctx;
    callback.on_success = resultSuccess;
    callback.on_error = resultError;
    return callback;
}

static AnyChatAuthDeviceListCallback_C makeAuthDeviceListCallback(CallbackContext* ctx) {
    AnyChatAuthDeviceListCallback_C callback{};
    callback.struct_size = sizeof(callback);
    callback.userdata = ctx;
    callback.on_success = authDeviceListSuccess;
    callback.on_error = authDeviceListError;
    return callback;
}

// Login
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeLogin(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring account,
    jstring password,
    jint deviceType,
    jstring clientVersion,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    JStringWrapper accountStr(env, account);
    JStringWrapper passwordStr(env, password);
    JStringWrapper clientVersionStr(env, clientVersion);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthTokenCallback_C authCb = makeAuthTokenCallback(ctx);

    int result = anychat_auth_login(
        authHandle,
        accountStr.c_str(),
        passwordStr.c_str(),
        static_cast<int32_t>(deviceType),
        clientVersionStr.c_str(),
        &authCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Login failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Register
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeRegister(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring phoneOrEmail,
    jstring password,
    jstring verifyCode,
    jint deviceType,
    jstring nickname,
    jstring clientVersion,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    JStringWrapper phoneOrEmailStr(env, phoneOrEmail);
    JStringWrapper passwordStr(env, password);
    JStringWrapper verifyCodeStr(env, verifyCode);
    JStringWrapper nicknameStr(env, nickname);
    JStringWrapper clientVersionStr(env, clientVersion);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthTokenCallback_C authCb = makeAuthTokenCallback(ctx);

    int result = anychat_auth_register(
        authHandle,
        phoneOrEmailStr.c_str(),
        passwordStr.c_str(),
        verifyCodeStr.c_str(),
        static_cast<int32_t>(deviceType),
        nicknameStr.c_str(),
        clientVersionStr.c_str(),
        &authCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Register failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Send verification code
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeSendCode(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring target,
    jint targetType,
    jint purpose,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    JStringWrapper targetStr(env, target);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatVerificationCodeCallback_C codeCb = makeVerificationCodeCallback(ctx);

    int result = anychat_auth_send_code(
        authHandle,
        targetStr.c_str(),
        static_cast<int32_t>(targetType),
        static_cast<int32_t>(purpose),
        &codeCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Send code failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Logout
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeLogout(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthResultCallback_C resultCb = makeResultCallback(ctx);

    int result = anychat_auth_logout(authHandle, &resultCb);

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Logout failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Refresh token
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeRefreshToken(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring refreshToken,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    JStringWrapper refreshTokenStr(env, refreshToken);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthTokenCallback_C authCb = makeAuthTokenCallback(ctx);

    int result = anychat_auth_refresh_token(
        authHandle,
        refreshTokenStr.c_str(),
        &authCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Refresh token failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Change password
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeChangePassword(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring oldPassword,
    jstring newPassword,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    JStringWrapper oldPasswordStr(env, oldPassword);
    JStringWrapper newPasswordStr(env, newPassword);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthResultCallback_C resultCb = makeResultCallback(ctx);

    int result = anychat_auth_change_password(
        authHandle,
        oldPasswordStr.c_str(),
        newPasswordStr.c_str(),
        &resultCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Change password failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Reset password
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeResetPassword(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring account,
    jstring verifyCode,
    jstring newPassword,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    JStringWrapper accountStr(env, account);
    JStringWrapper verifyCodeStr(env, verifyCode);
    JStringWrapper newPasswordStr(env, newPassword);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthResultCallback_C resultCb = makeResultCallback(ctx);

    int result = anychat_auth_reset_password(
        authHandle,
        accountStr.c_str(),
        verifyCodeStr.c_str(),
        newPasswordStr.c_str(),
        &resultCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Reset password failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Get auth device list
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeGetDeviceList(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthDeviceListCallback_C deviceListCb = makeAuthDeviceListCallback(ctx);

    int result = anychat_auth_get_device_list(authHandle, &deviceListCb);

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Get device list failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Logout specified device
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Auth_nativeLogoutDevice(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring deviceId,
    jobject callback
) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    JStringWrapper deviceIdStr(env, deviceId);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatAuthResultCallback_C resultCb = makeResultCallback(ctx);

    int result = anychat_auth_logout_device(authHandle, deviceIdStr.c_str(), &resultCb);

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Logout device failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Is logged in
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_anychat_sdk_Auth_nativeIsLoggedIn(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    int isLoggedIn = anychat_auth_is_logged_in(authHandle);
    return (jboolean)isLoggedIn;

    JNI_CATCH(env)
    return JNI_FALSE;
}

// Get current token
extern "C"
JNIEXPORT jobject JNICALL
Java_com_anychat_sdk_Auth_nativeGetCurrentToken(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto authHandle = reinterpret_cast<AnyChatAuthHandle>(handle);
    AnyChatAuthToken_C token = {};

    int result = anychat_auth_get_current_token(authHandle, &token);
    if (result == ANYCHAT_OK) {
        return convertAuthToken(env, token);
    }

    return nullptr;

    JNI_CATCH(env)
    return nullptr;
}
