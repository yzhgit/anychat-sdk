#include "jni_helpers.h"
#include <map>
#include <memory>
#include <mutex>

using namespace anychat::jni;

extern JavaVM* g_jvm;

static std::map<jlong, std::unique_ptr<CallbackContext>> g_convListenerCallbacks;
static std::mutex g_convListenerMutex;

static void convListSuccess(void* userdata, const AnyChatConversationList_C* list) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onConversationList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = list ? convertConversationList(env, list) : nullptr;
        env->CallVoidMethod(ctx->callback, mid, listObj, nullptr);
        if (listObj) env->DeleteLocalRef(listObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void convListError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onConversationList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void convSuccess(void* userdata) {
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

static void convError(void* userdata, int code, const char* error) {
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

static void convUpdatedCallback(void* userdata, const AnyChatConversation_C* conversation) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback || !conversation) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onConversationUpdated",
        "(Lcom/anychat/sdk/models/Conversation;)V");

    if (mid) {
        jobject convObj = convertConversation(env, *conversation);
        if (convObj) {
            env->CallVoidMethod(ctx->callback, mid, convObj);
            env->DeleteLocalRef(convObj);
        }
    }

    env->DeleteLocalRef(cls);
}

static AnyChatConvListCallback_C makeConvListCallback(CallbackContext* ctx) {
    AnyChatConvListCallback_C callback{};
    callback.struct_size = sizeof(callback);
    callback.userdata = ctx;
    callback.on_success = convListSuccess;
    callback.on_error = convListError;
    return callback;
}

static AnyChatConvCallback_C makeConvCallback(CallbackContext* ctx) {
    AnyChatConvCallback_C callback{};
    callback.struct_size = sizeof(callback);
    callback.userdata = ctx;
    callback.on_success = convSuccess;
    callback.on_error = convError;
    return callback;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Conversation_nativeGetList(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto convHandle = reinterpret_cast<AnyChatConvHandle>(handle);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatConvListCallback_C listCb = makeConvListCallback(ctx);

    int result = anychat_conv_get_list(convHandle, &listCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Get conversation list failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Conversation_nativeMarkRead(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring convId,
    jobject callback
) {
    JNI_TRY(env)

    auto convHandle = reinterpret_cast<AnyChatConvHandle>(handle);
    JStringWrapper convIdStr(env, convId);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatConvCallback_C convCb = makeConvCallback(ctx);

    int result = anychat_conv_mark_all_read(convHandle, convIdStr.c_str(), &convCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Mark conversation read failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Conversation_nativeSetPinned(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring convId,
    jboolean pinned,
    jobject callback
) {
    JNI_TRY(env)

    auto convHandle = reinterpret_cast<AnyChatConvHandle>(handle);
    JStringWrapper convIdStr(env, convId);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatConvCallback_C convCb = makeConvCallback(ctx);

    int result = anychat_conv_set_pinned(convHandle, convIdStr.c_str(), pinned ? 1 : 0, &convCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Set conversation pinned failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Conversation_nativeSetMuted(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring convId,
    jboolean muted,
    jobject callback
) {
    JNI_TRY(env)

    auto convHandle = reinterpret_cast<AnyChatConvHandle>(handle);
    JStringWrapper convIdStr(env, convId);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatConvCallback_C convCb = makeConvCallback(ctx);

    int result = anychat_conv_set_muted(convHandle, convIdStr.c_str(), muted ? 1 : 0, &convCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Set conversation muted failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Conversation_nativeDelete(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring convId,
    jobject callback
) {
    JNI_TRY(env)

    auto convHandle = reinterpret_cast<AnyChatConvHandle>(handle);
    JStringWrapper convIdStr(env, convId);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatConvCallback_C convCb = makeConvCallback(ctx);

    int result = anychat_conv_delete(convHandle, convIdStr.c_str(), &convCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Delete conversation failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Conversation_nativeSetUpdatedCallback(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto convHandle = reinterpret_cast<AnyChatConvHandle>(handle);
    std::unique_ptr<CallbackContext> oldCtx;

    {
        std::lock_guard<std::mutex> lock(g_convListenerMutex);
        auto existing = g_convListenerCallbacks.find(handle);
        if (existing != g_convListenerCallbacks.end()) {
            oldCtx = std::move(existing->second);
            g_convListenerCallbacks.erase(existing);
        }

        if (callback) {
            jobject globalCallback = env->NewGlobalRef(callback);
            auto ctx = std::make_unique<CallbackContext>(g_jvm, globalCallback);

            AnyChatConvListener_C listener{};
            listener.struct_size = sizeof(listener);
            listener.userdata = ctx.get();
            listener.on_conversation_updated = convUpdatedCallback;

            int result = anychat_conv_set_listener(convHandle, &listener);
            if (result != ANYCHAT_OK) {
                LOGE("Set conversation listener failed with error code: %d", result);
                return;
            }

            g_convListenerCallbacks[handle] = std::move(ctx);
        } else {
            int result = anychat_conv_set_listener(convHandle, nullptr);
            if (result != ANYCHAT_OK) {
                LOGE("Clear conversation listener failed with error code: %d", result);
            }
        }
    }

    JNI_CATCH(env)
}
