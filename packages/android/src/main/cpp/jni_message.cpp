#include "jni_helpers.h"
#include "anychat_c.h"

using namespace anychat::jni;

extern JavaVM* g_jvm;

// Message callback wrapper
static void messageCallback(void* userdata, int success, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onResult", "(ZLjava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, (jboolean)success, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

// Message list callback wrapper
static void messageListCallback(void* userdata, const AnyChatMessageList_C* list, const char* error) {
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
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, listObj, errorStr);

        if (listObj) env->DeleteLocalRef(listObj);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

// Message received callback wrapper
static void messageReceivedCallback(void* userdata, const AnyChatMessage_C* message) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

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

    int result = anychat_message_send_text(
        msgHandle,
        sessionIdStr.c_str(),
        contentStr.c_str(),
        ctx,
        messageCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
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

    int result = anychat_message_get_history(
        msgHandle,
        sessionIdStr.c_str(),
        (int64_t)beforeTimestampMs,
        (int)limit,
        ctx,
        messageListCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
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

    int result = anychat_message_mark_read(
        msgHandle,
        sessionIdStr.c_str(),
        messageIdStr.c_str(),
        ctx,
        messageCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
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

    if (callback) {
        jobject globalCallback = env->NewGlobalRef(callback);
        auto* ctx = new CallbackContext(g_jvm, globalCallback);
        anychat_message_set_received_callback(msgHandle, ctx, messageReceivedCallback);
    } else {
        anychat_message_set_received_callback(msgHandle, nullptr, nullptr);
    }

    JNI_CATCH(env)
}
