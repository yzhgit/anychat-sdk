# AnyChat SDK ProGuard Rules

# Keep native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep AnyChat SDK public API
-keep class com.anychat.sdk.** { *; }

# Keep Kotlin coroutines
-keepnames class kotlinx.coroutines.internal.MainDispatcherFactory {}
-keepnames class kotlinx.coroutines.CoroutineExceptionHandler {}
-keepclassmembernames class kotlinx.** {
    volatile <fields>;
}

# Keep data classes
-keepclassmembers class com.anychat.sdk.models.** {
    *;
}
