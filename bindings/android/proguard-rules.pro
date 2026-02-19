# Add project specific ProGuard rules here.

# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep AnyChat SDK classes
-keep class com.anychat.sdk.** { *; }

# Keep model classes (used in JNI)
-keep class com.anychat.sdk.models.** { *; }

# Keep callback interfaces
-keep interface com.anychat.sdk.*Callback { *; }

# Keep Kotlin metadata
-keep class kotlin.Metadata { *; }

# Keep coroutines
-keepnames class kotlinx.coroutines.internal.MainDispatcherFactory {}
-keepnames class kotlinx.coroutines.CoroutineExceptionHandler {}
-keepnames class kotlinx.coroutines.android.AndroidExceptionPreHandler {}
-keepnames class kotlinx.coroutines.android.AndroidDispatcherFactory {}

# Keep coroutine internal state machines
-keepclassmembernames class kotlinx.** {
    volatile <fields>;
}
