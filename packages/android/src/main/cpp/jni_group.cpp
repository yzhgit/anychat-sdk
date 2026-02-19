#include "jni_helpers.h"
#include "anychat_c.h"
#include <vector>

using namespace anychat::jni;

extern JavaVM* g_jvm;

// Group list callback wrapper
static void groupListCallback(void* userdata, const AnyChatGroupList_C* list, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onGroupList",
        "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = nullptr;
        if (list) {
            listObj = convertGroupList(env, list);
        }
        jstring errorStr = toJString(env, error);
        env->CallVoidMethod(ctx->callback, mid, listObj, errorStr);

        if (listObj) env->DeleteLocalRef(listObj);
        if (errorStr) env->DeleteLocalRef(errorStr);
    }

    env->DeleteLocalRef(cls);
    delete ctx;
}

// Group callback wrapper
static void groupCallback(void* userdata, int success, const char* error) {
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

// Group member callback wrapper
static void groupMemberCallback(void* userdata, const AnyChatGroupMemberList_C* list, const char* error) {
    auto* ctx = static_cast<CallbackContext*>(userdata);
    if (!ctx || !ctx->callback) return;

    JNIEnv* env = getEnvForCallback(ctx->jvm);
    if (!env) return;

    jclass cls = env->GetObjectClass(ctx->callback);
    jmethodID mid = env->GetMethodID(cls, "onGroupMemberList",
        "(Ljava/util/List;Ljava/lang/String;)V");

    if (mid) {
        jobject listObj = nullptr;
        if (list && list->count > 0) {
            jclass arrayListClass = env->FindClass("java/util/ArrayList");
            jmethodID arrayListInit = env->GetMethodID(arrayListClass, "<init>", "(I)V");
            jmethodID arrayListAdd = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

            listObj = env->NewObject(arrayListClass, arrayListInit, list->count);

            for (int i = 0; i < list->count; i++) {
                jobject memberObj = convertGroupMember(env, list->items[i]);
                if (memberObj) {
                    env->CallBooleanMethod(listObj, arrayListAdd, memberObj);
                    env->DeleteLocalRef(memberObj);
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

// Get group list
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

    int result = anychat_group_get_list(groupHandle, ctx, groupListCallback);

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Get group list failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Create group
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

    // Convert Java String array to C array
    int memberCount = env->GetArrayLength(memberIds);
    std::vector<const char*> memberArray;
    std::vector<JStringWrapper*> wrappers;

    for (int i = 0; i < memberCount; i++) {
        jstring jstr = (jstring)env->GetObjectArrayElement(memberIds, i);
        auto* wrapper = new JStringWrapper(env, jstr);
        wrappers.push_back(wrapper);
        memberArray.push_back(wrapper->c_str());
        env->DeleteLocalRef(jstr);
    }
    memberArray.push_back(nullptr); // NULL-terminated

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_group_create(
        groupHandle,
        nameStr.c_str(),
        memberArray.data(),
        memberCount,
        ctx,
        groupCallback
    );

    // Clean up wrappers
    for (auto* wrapper : wrappers) {
        delete wrapper;
    }

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Create group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Join group
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

    int result = anychat_group_join(
        groupHandle,
        groupIdStr.c_str(),
        messageStr.c_str(),
        ctx,
        groupCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Join group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Invite to group
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

    // Convert Java String array to C array
    int userCount = env->GetArrayLength(userIds);
    std::vector<const char*> userArray;
    std::vector<JStringWrapper*> wrappers;

    for (int i = 0; i < userCount; i++) {
        jstring jstr = (jstring)env->GetObjectArrayElement(userIds, i);
        auto* wrapper = new JStringWrapper(env, jstr);
        wrappers.push_back(wrapper);
        userArray.push_back(wrapper->c_str());
        env->DeleteLocalRef(jstr);
    }
    userArray.push_back(nullptr);

    jobject globalCallback = env->NewGlobalRef(callback);
    auto* ctx = new CallbackContext(g_jvm, globalCallback);

    int result = anychat_group_invite(
        groupHandle,
        groupIdStr.c_str(),
        userArray.data(),
        userCount,
        ctx,
        groupCallback
    );

    // Clean up wrappers
    for (auto* wrapper : wrappers) {
        delete wrapper;
    }

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Invite to group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Quit group
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

    int result = anychat_group_quit(groupHandle, groupIdStr.c_str(), ctx, groupCallback);

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Quit group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Update group
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

    int result = anychat_group_update(
        groupHandle,
        groupIdStr.c_str(),
        nameStr.c_str(),
        avatarUrlStr.c_str(),
        ctx,
        groupCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Update group failed with error code: %d", result);
    }

    JNI_CATCH(env)
}

// Get group members
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

    int result = anychat_group_get_members(
        groupHandle,
        groupIdStr.c_str(),
        (int)page,
        (int)pageSize,
        ctx,
        groupMemberCallback
    );

    if (result != ANYCHAT_OK) {
        delete ctx;
        env->DeleteGlobalRef(globalCallback);
        LOGE("Get group members failed with error code: %d", result);
    }

    JNI_CATCH(env)
}
