#include "jni_helpers.h"

using namespace anychat::jni;

extern JavaVM* g_jvm;

static void friendListSuccess(void* userdata, const AnyChatFriendList_C* list) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onFriendList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = list ? convertFriendList(env, list) : nullptr;
        env->CallVoidMethod(ctx->callback, mid, listObj, nullptr);
        if (listObj) env->DeleteLocalRef(listObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void friendListError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onFriendList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void friendRequestListSuccess(void* userdata, const AnyChatFriendRequestList_C* list) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onFriendRequestList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = nullptr;
        if (list && list->count > 0) {
            jclass arrayListClass = env->FindClass("java/util/ArrayList");
            jmethodID arrayListInit = env->GetMethodID(arrayListClass, "<init>", "(I)V");
            jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

            listObj = env->NewObject(arrayListClass, arrayListInit, list->count);
            for (int i = 0; i < list->count; ++i) {
                jobject requestObj = convertFriendRequest(env, list->items[i]);
                if (requestObj) {
                    env->CallBooleanMethod(listObj, arrayListAdd, requestObj);
                    env->DeleteLocalRef(requestObj);
                }
            }
            env->DeleteLocalRef(arrayListClass);
        }

        env->CallVoidMethod(ctx->callback, mid, listObj, nullptr);
        if (listObj) env->DeleteLocalRef(listObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void friendRequestListError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onFriendRequestList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void friendSuccess(void* userdata) {
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

static void friendError(void* userdata, int code, const char* error) {
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

static AnyChatFriendListCallback_C makeFriendListCallback(CallbackContext* ctx) {
    AnyChatFriendListCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = friendListSuccess;
    callback.on_error = friendListError;
    return callback;
}

static AnyChatFriendRequestListCallback_C makeFriendRequestListCallback(CallbackContext* ctx) {
    AnyChatFriendRequestListCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = friendRequestListSuccess;
    callback.on_error = friendRequestListError;
    return callback;
}

static AnyChatFriendCallback_C makeFriendCallback(CallbackContext* ctx) {
    AnyChatFriendCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = friendSuccess;
    callback.on_error = friendError;
    return callback;
}

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
    AnyChatFriendListCallback_C friendListCb = makeFriendListCallback(ctx);

    int result = anychat_friend_get_list(friendHandle, &friendListCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Get friend list failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeSendRequest(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring toUserId,
    jstring message,
    jint source,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);
    JStringWrapper toUserIdStr(env, toUserId);
    JStringWrapper messageStr(env, message);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatFriendCallback_C friendCb = makeFriendCallback(ctx);

    int result = anychat_friend_add(
        friendHandle,
        toUserIdStr.c_str(),
        messageStr.c_str(),
        static_cast<int32_t>(source),
        &friendCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Send friend request failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeHandleRequest(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jlong requestId,
    jint action,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatFriendCallback_C friendCb = makeFriendCallback(ctx);

    int result = anychat_friend_handle_request(
        friendHandle,
        (int64_t)requestId,
        static_cast<int32_t>(action),
        &friendCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Handle friend request failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Friend_nativeGetPendingRequests(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jint requestType,
    jobject callback
) {
    JNI_TRY(env)

    auto friendHandle = reinterpret_cast<AnyChatFriendHandle>(handle);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatFriendRequestListCallback_C requestListCb = makeFriendRequestListCallback(ctx);

    int result = anychat_friend_get_requests(friendHandle, static_cast<int32_t>(requestType), &requestListCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Get pending friend requests failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

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
    AnyChatFriendCallback_C friendCb = makeFriendCallback(ctx);

    int result = anychat_friend_delete(friendHandle, friendIdStr.c_str(), &friendCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Delete friend failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

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
    AnyChatFriendCallback_C friendCb = makeFriendCallback(ctx);

    int result = anychat_friend_update_remark(friendHandle, friendIdStr.c_str(), remarkStr.c_str(), &friendCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Update friend remark failed with error code: %d", result);
    }

    JNI_CATCH(env)
}
