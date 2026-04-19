#include "jni_helpers.h"
#include <map>
#include <memory>
#include <mutex>

using namespace anychat::jni;

extern JavaVM* g_jvm;

static std::map<jlong, std::unique_ptr<CallbackContext>> g_messageListenerCallbacks;
static std::mutex g_messageListenerMutex;

static void messageSuccess(void* userdata) {
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

static void messageError(void* userdata, int code, const char* error) {
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

static void messageListSuccess(void* userdata, const AnyChatMessageList_C* list) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onMessageList",
        "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = nullptr;
        if (list) {
            listObj = convertMessageList(env, list);
        }
        env->CallVoidMethod(ctx->callback, mid, listObj, nullptr);

        if (listObj) env->DeleteLocalRef(listObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void messageListError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onMessageList",
        "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void messageReceivedCallback(void* userdata, const AnyChatMessage_C* message) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback || !message) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onMessageReceived",
        "(Lcom/anychat/sdk/models/Message;)V");

    if (mid) {
        jobject msgObj = convertMessage(env, *message);
        if (msgObj) {
            env->CallVoidMethod(ctx->callback, mid, msgObj);
            env->DeleteLocalRef(msgObj);
        }
    }

    env->DeleteLocalRef(cls);
}

static AnyChatMessageCallback_C makeMessageCallback(CallbackContext* ctx) {
    AnyChatMessageCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = messageSuccess;
    callback.on_error = messageError;
    return callback;
}

static AnyChatMessageListCallback_C makeMessageListCallback(CallbackContext* ctx) {
    AnyChatMessageListCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = messageListSuccess;
    callback.on_error = messageListError;
    return callback;
}

// Send text message
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Message_nativeSendText(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring sessionId,
    jstring content,
    jobject callback
) {
    JNI_TRY(env)

    auto msgHandle = reinterpret_cast<AnyChatMessageHandle>(handle);
    JStringWrapper sessionIdStr(env, sessionId);
    JStringWrapper contentStr(env, content);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatMessageCallback_C messageCb = makeMessageCallback(ctx);

    int result = anychat_message_send_text(
        msgHandle,
        sessionIdStr.c_str(),
        contentStr.c_str(),
        &messageCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Send text failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Get message history
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Message_nativeGetHistory(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring sessionId,
    jlong beforeTimestampMs,
    jint limit,
    jobject callback
) {
    JNI_TRY(env)

    auto msgHandle = reinterpret_cast<AnyChatMessageHandle>(handle);
    JStringWrapper sessionIdStr(env, sessionId);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatMessageListCallback_C listCb = makeMessageListCallback(ctx);

    int result = anychat_message_get_history(
        msgHandle,
        sessionIdStr.c_str(),
        (int64_t)beforeTimestampMs,
        (int)limit,
        &listCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Get history failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Mark message as read
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Message_nativeMarkRead(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring sessionId,
    jstring messageId,
    jobject callback
) {
    JNI_TRY(env)

    auto msgHandle = reinterpret_cast<AnyChatMessageHandle>(handle);
    JStringWrapper sessionIdStr(env, sessionId);
    JStringWrapper messageIdStr(env, messageId);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatMessageCallback_C messageCb = makeMessageCallback(ctx);

    int result = anychat_message_mark_read(
        msgHandle,
        sessionIdStr.c_str(),
        messageIdStr.c_str(),
        &messageCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Mark read failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Set message received callback
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Message_nativeSetReceivedCallback(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto msgHandle = reinterpret_cast<AnyChatMessageHandle>(handle);
    std::unique_ptr<CallbackContext> oldCtx;

    {
        std::lock_guard<std::mutex> lock(g_messageListenerMutex);
        auto existing = g_messageListenerCallbacks.find(handle);
        if (existing != g_messageListenerCallbacks.end()) {
            oldCtx = std::move(existing->second);
            g_messageListenerCallbacks.erase(existing);
        }

        if (callback) {
            jobject globalCallback = env->NewGlobalRef(callback);
            auto ctx = std::make_unique<CallbackContext>(g_jvm, globalCallback);

            AnyChatMessageListener_C listener{};
            listener.userdata = ctx.get();
            listener.on_message_received = messageReceivedCallback;

            int result = anychat_message_set_listener(msgHandle, &listener);
            if (result != ANYCHAT_OK) {
                LOGE("Set message listener failed with error code: %d", result);
                return;
            }

            g_messageListenerCallbacks[handle] = std::move(ctx);
        } else {
            int result = anychat_message_set_listener(msgHandle, nullptr);
            if (result != ANYCHAT_OK) {
                LOGE("Clear message listener failed with error code: %d", result);
            }
        }
    }

    JNI_CATCH(env)
}
