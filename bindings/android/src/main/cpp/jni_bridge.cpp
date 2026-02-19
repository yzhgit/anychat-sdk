#include "jni_helpers.h"
#include "anychat_c.h"
#include <map>
#include <mutex>

using namespace anychat::jni;

// Global JavaVM pointer
static JavaVM* g_jvm = nullptr;

// Client handle map (native pointer -> handle)
static std::map<jlong, AnyChatClientHandle> g_clientHandles;
static std::mutex g_clientMutex;

// JNI_OnLoad - called when library is loaded
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    LOGI("AnyChatJNI library loaded");
    return JNI_VERSION_1_6;
}

// JNI_OnUnload - called when library is unloaded
extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    LOGI("AnyChatJNI library unloaded");
    g_jvm = nullptr;
}

// Connection state callback wrapper
static void connectionStateCallback(void* userdata, int state) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onConnectionStateChanged", "(I)V");
    if (mid) {
        env->CallVoidMethod(ctx->callback, mid, (jint)state);
    }
    env->DeleteLocalRef(cls);
}

// Create client
extern "C"
JNIEXPORT jlong JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeCreate(
    JNIEnv* env,
    jobject thiz,
    jstring gatewayUrl,
    jstring apiBaseUrl,
    jstring deviceId,
    jstring dbPath,
    jint connectTimeoutMs,
    jint maxReconnectAttempts,
    jboolean autoReconnect
) {
    JNI_TRY(env)

    JStringWrapper gateway(env, gatewayUrl);
    JStringWrapper apiBase(env, apiBaseUrl);
    JStringWrapper device(env, deviceId);
    JStringWrapper db(env, dbPath);

    AnyChatClientConfig_C config = {};
    config.gateway_url = gateway.c_str();
    config.api_base_url = apiBase.c_str();
    config.device_id = device.c_str();
    config.db_path = db.c_str();
    config.connect_timeout_ms = connectTimeoutMs;
    config.max_reconnect_attempts = maxReconnectAttempts;
    config.auto_reconnect = autoReconnect ? 1 : 0;

    AnyChatClientHandle handle = anychat_client_create(&config);
    if (!handle) {
        const char* error = anychat_get_last_error();
        LOGE("Failed to create client: %s", error);
        jclass exClass = env->FindClass("java/lang/RuntimeException");
        env->ThrowNew(exClass, error);
        return 0;
    }

    jlong ptrValue = reinterpret_cast<jlong>(handle);

    {
        std::lock_guard<std::mutex> lock(g_clientMutex);
        g_clientHandles[ptrValue] = handle;
    }

    LOGI("Client created: %p", handle);
    return ptrValue;

    JNI_CATCH(env)
    return 0;
}

// Destroy client
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    AnyChatClientHandle clientHandle = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_clientMutex);
        auto it = g_clientHandles.find(handle);
        if (it != g_clientHandles.end()) {
            clientHandle = it->second;
            g_clientHandles.erase(it);
        }
    }

    if (clientHandle) {
        anychat_client_destroy(clientHandle);
        LOGI("Client destroyed: %p", clientHandle);
    }

    JNI_CATCH(env)
}

// Connect
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeConnect(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    anychat_client_connect(clientHandle);
    LOGI("Client connect called");

    JNI_CATCH(env)
}

// Disconnect
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeDisconnect(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    anychat_client_disconnect(clientHandle);
    LOGI("Client disconnect called");

    JNI_CATCH(env)
}

// Get connection state
extern "C"
JNIEXPORT jint JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeGetConnectionState(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    int state = anychat_client_get_connection_state(clientHandle);
    return (jint)state;

    JNI_CATCH(env)
    return 0;
}

// Set connection callback
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeSetConnectionCallback(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);

    if (callback) {
        jobject globalCallback = env->NewGlobalRef(callback);
        auto* ctx = new CallbackContext(g_jvm, globalCallback);
        anychat_client_set_connection_callback(clientHandle, ctx, connectionStateCallback);
    } else {
        anychat_client_set_connection_callback(clientHandle, nullptr, nullptr);
    }

    JNI_CATCH(env)
}

// Get auth manager
extern "C"
JNIEXPORT jlong JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeGetAuth(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    AnyChatAuthHandle authHandle = anychat_client_get_auth(clientHandle);
    return reinterpret_cast<jlong>(authHandle);

    JNI_CATCH(env)
    return 0;
}

// Get message manager
extern "C"
JNIEXPORT jlong JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeGetMessage(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    AnyChatMessageHandle msgHandle = anychat_client_get_message(clientHandle);
    return reinterpret_cast<jlong>(msgHandle);

    JNI_CATCH(env)
    return 0;
}

// Get conversation manager
extern "C"
JNIEXPORT jlong JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeGetConversation(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    AnyChatConvHandle convHandle = anychat_client_get_conversation(clientHandle);
    return reinterpret_cast<jlong>(convHandle);

    JNI_CATCH(env)
    return 0;
}

// Get friend manager
extern "C"
JNIEXPORT jlong JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeGetFriend(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    AnyChatFriendHandle friendHandle = anychat_client_get_friend(clientHandle);
    return reinterpret_cast<jlong>(friendHandle);

    JNI_CATCH(env)
    return 0;
}

// Get group manager
extern "C"
JNIEXPORT jlong JNICALL
Java_com_anychat_sdk_AnyChatClient_nativeGetGroup(JNIEnv* env, jobject thiz, jlong handle) {
    JNI_TRY(env)

    auto clientHandle = reinterpret_cast<AnyChatClientHandle>(handle);
    AnyChatGroupHandle groupHandle = anychat_client_get_group(clientHandle);
    return reinterpret_cast<jlong>(groupHandle);

    JNI_CATCH(env)
    return 0;
}
