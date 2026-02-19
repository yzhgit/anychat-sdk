#include "jni_helpers.h"
#include "anychat_c.h"

using namespace anychat::jni;

extern JavaVM* g_jvm;

// Friend list callback wrapper
static void friendListCallback(void* userdata, const AnyChatFriendList_C* list, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onFriendList",
        "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = nullptr;
        if (list) {
            listObj = convertFriendList(env, list);
        }
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, listObj, errorStr);

        if (listObj) env->DeleteLocalRef(listObj);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

// Friend request list callback wrapper
static void friendRequestListCallback(void* userdata, const AnyChatFriendRequestList_C* list, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onFriendRequestList",
        "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = nullptr;
        if (list && list->count > 0) {
            // Convert to Java list
            jclass arrayListClass = env->FindClass("java/util/ArrayList");
            jmethodID arrayListInit = env->GetMethodID(arrayListClass, "<init>", "(I)V");
            jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

            listObj = env->NewObject(arrayListClass, arrayListInit, list->count);

            for (int i = 0; i < list->count; i++) {
                jobject requestObj = convertFriendRequest(env, list->items[i]);
                if (requestObj) {
                    env->CallBooleanMethod(listObj, arrayListAdd, requestObj);
                    env->DeleteLocalRef(requestObj);
                }
            }

            env->DeleteLocalRef(arrayListClass);
        }

        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, listObj, errorStr);

        if (listObj) env->DeleteLocalRef(listObj);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

// Friend callback wrapper
static void friendCallback(void* userdata, int success, const char* error) {
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

// Get friend list
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeGetList(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_friend_get_list(friendHandle, ctx, friendListCallback);

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Get friend list failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Send friend request
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeSendRequest(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring toUserId,
    jstring message,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);
    JStringWrapper toUserIdStr(env, toUserId);
    JStringWrapper messageStr(env, message);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_friend_send_request(
        friendHandle,
        toUserIdStr.c_str(),
        messageStr.c_str(),
        ctx,
        friendCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Send friend request failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Handle friend request
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeHandleRequest(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jlong requestId,
    jboolean accept,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_friend_handle_request(
        friendHandle,
        (int64_t)requestId,
        accept ? 1 : 0,
        ctx,
        friendCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Handle friend request failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Get pending friend requests
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeGetPendingRequests(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_friend_get_pending_requests(friendHandle, ctx, friendRequestListCallback);

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Get pending requests failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Delete friend
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeDelete(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring friendId,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);
    JStringWrapper friendIdStr(env, friendId);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_friend_delete(friendHandle, friendIdStr.c_str(), ctx, friendCallback);

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Delete friend failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Update friend remark
extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeUpdateRemark(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring friendId,
    jstring remark,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);
    JStringWrapper friendIdStr(env, friendId);
    JStringWrapper remarkStr(env, remark);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_friend_update_remark(
        friendHandle,
        friendIdStr.c_str(),
        remarkStr.c_str(),
        ctx,
        friendCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Update friend remark failed with error code: %d", result);
    }

    JNI_CATCH(env)
}
