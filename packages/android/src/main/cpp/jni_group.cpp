#include "jni_helpers.h"
#include <vector>

using namespace anychat::jni;

extern JavaVM* g_jvm;

static void groupListSuccess(void* userdata, const AnyChatGroupList_C* list) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onGroupList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = list ? convertGroupList(env, list) : nullptr;
        env->CallVoidMethod(ctx->callback, mid, listObj, nullptr);
        if (listObj) env->DeleteLocalRef(listObj);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void groupListError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onGroupList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void groupSuccess(void* userdata) {
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

static void groupError(void* userdata, int code, const char* error) {
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

static void groupMemberListSuccess(void* userdata, const AnyChatGroupMemberList_C* list) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onGroupMemberList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = nullptr;
        if (list && list->count > 0) {
            jclass arrayListClass = env->FindClass("java/util/ArrayList");
            jmethodID arrayListInit = env->GetMethodID(arrayListClass, "<init>", "(I)V");
            jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

            listObj = env->NewObject(arrayListClass, arrayListInit, list->count);
            for (int i = 0; i < list->count; ++i) {
                jobject memberObj = convertGroupMember(env, list->items[i]);
                if (memberObj) {
                    env->CallBooleanMethod(listObj, arrayListAdd, memberObj);
                    env->DeleteLocalRef(memberObj);
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

static void groupMemberListError(void* userdata, int code, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onGroupMemberList", "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, nullptr, errorStr);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

static void groupInfoSuccess(void* userdata, const AnyChatGroup_C* group) {
    groupSuccess(userdata);
}

static AnyChatGroupListCallback_C makeGroupListCallback(CallbackContext* ctx) {
    AnyChatGroupListCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = groupListSuccess;
    callback.on_error = groupListError;
    return callback;
}

static AnyChatGroupCallback_C makeGroupCallback(CallbackContext* ctx) {
    AnyChatGroupCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = groupSuccess;
    callback.on_error = groupError;
    return callback;
}

static AnyChatGroupInfoCallback_C makeGroupInfoCallback(CallbackContext* ctx) {
    AnyChatGroupInfoCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = groupInfoSuccess;
    callback.on_error = groupError;
    return callback;
}

static AnyChatGroupMemberListCallback_C makeGroupMemberListCallback(CallbackContext* ctx) {
    AnyChatGroupMemberListCallback_C callback{};
    callback.userdata = ctx;
    callback.on_success = groupMemberListSuccess;
    callback.on_error = groupMemberListError;
    return callback;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Group_nativeGetList(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jobject callback
) {
    JNI_TRY(env)

    auto groupHandle = reinterpret_cast<AnyChatGroupHandle>(handle);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatGroupListCallback_C groupListCb = makeGroupListCallback(ctx);

    int result = anychat_group_get_list(groupHandle, &groupListCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Get group list failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Group_nativeCreate(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring name,
    jobjectArray memberIds,
    jobject callback
) {
    JNI_TRY(env)

    auto groupHandle = reinterpret_cast<AnyChatGroupHandle>(handle);
    JStringWrapper nameStr(env, name);

    int memberCount = env->GetArrayLength(memberIds);
    std::vector<const char*> memberArray;
    std::vector<JStringWrapper*> wrappers;
    memberArray.reserve(static_cast<size_t>(memberCount + 1));

    for (int i = 0; i < memberCount; ++i) {
        jstring jstr = static_cast<jstring>(env->GetObjectArrayElement(memberIds, i));
        auto* wrapper = new JStringWrapper(env, jstr);
        wrappers.push_back(wrapper);
        memberArray.push_back(wrapper->c_str());
        env->DeleteLocalRef(jstr);
    }
    memberArray.push_back(nullptr);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatGroupInfoCallback_C groupInfoCb = makeGroupInfoCallback(ctx);

    int result = anychat_group_create(
        groupHandle,
        nameStr.c_str(),
        memberArray.data(),
        memberCount,
        &groupInfoCb
    );

    for (auto* wrapper : wrappers) {
        delete wrapper;
    }

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Create group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Group_nativeJoin(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring groupId,
    jstring message,
    jobject callback
) {
    JNI_TRY(env)

    auto groupHandle = reinterpret_cast<AnyChatGroupHandle>(handle);
    JStringWrapper groupIdStr(env, groupId);
    JStringWrapper messageStr(env, message);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatGroupCallback_C groupCb = makeGroupCallback(ctx);

    int result = anychat_group_join(groupHandle, groupIdStr.c_str(), messageStr.c_str(), &groupCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Join group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Group_nativeInvite(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring groupId,
    jobjectArray userIds,
    jobject callback
) {
    JNI_TRY(env)

    auto groupHandle = reinterpret_cast<AnyChatGroupHandle>(handle);
    JStringWrapper groupIdStr(env, groupId);

    int userCount = env->GetArrayLength(userIds);
    std::vector<const char*> userArray;
    std::vector<JStringWrapper*> wrappers;
    userArray.reserve(static_cast<size_t>(userCount + 1));

    for (int i = 0; i < userCount; ++i) {
        jstring jstr = static_cast<jstring>(env->GetObjectArrayElement(userIds, i));
        auto* wrapper = new JStringWrapper(env, jstr);
        wrappers.push_back(wrapper);
        userArray.push_back(wrapper->c_str());
        env->DeleteLocalRef(jstr);
    }
    userArray.push_back(nullptr);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatGroupCallback_C groupCb = makeGroupCallback(ctx);

    int result = anychat_group_invite(groupHandle, groupIdStr.c_str(), userArray.data(), userCount, &groupCb);

    for (auto* wrapper : wrappers) {
        delete wrapper;
    }

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Invite to group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Group_nativeQuit(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring groupId,
    jobject callback
) {
    JNI_TRY(env)

    auto groupHandle = reinterpret_cast<AnyChatGroupHandle>(handle);
    JStringWrapper groupIdStr(env, groupId);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatGroupCallback_C groupCb = makeGroupCallback(ctx);

    int result = anychat_group_quit(groupHandle, groupIdStr.c_str(), &groupCb);
    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Quit group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Group_nativeUpdate(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring groupId,
    jstring name,
    jstring avatarUrl,
    jobject callback
) {
    JNI_TRY(env)

    auto groupHandle = reinterpret_cast<AnyChatGroupHandle>(handle);
    JStringWrapper groupIdStr(env, groupId);
    JStringWrapper nameStr(env, name);
    JStringWrapper avatarUrlStr(env, avatarUrl);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatGroupCallback_C groupCb = makeGroupCallback(ctx);

    int result = anychat_group_update(
        groupHandle,
        groupIdStr.c_str(),
        nameStr.c_str(),
        avatarUrlStr.c_str(),
        &groupCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Update group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

extern "C"
JNIEXPORT void JNICALL
Java_com_anychat_sdk_Group_nativeGetMembers(
    JNIEnv* env,
    jobject thiz,
    jlong handle,
    jstring groupId,
    jint page,
    jint pageSize,
    jobject callback
) {
    JNI_TRY(env)

    auto groupHandle = reinterpret_cast<AnyChatGroupHandle>(handle);
    JStringWrapper groupIdStr(env, groupId);
    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);
    AnyChatGroupMemberListCallback_C memberListCb = makeGroupMemberListCallback(ctx);

    int result = anychat_group_get_members(
        groupHandle,
        groupIdStr.c_str(),
        static_cast<int>(page),
        static_cast<int>(pageSize),
        &memberListCb
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        LOGE("Get group members failed with error code: %d", result);
    }

    JNI_CATCH(env)
}
