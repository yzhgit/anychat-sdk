import 'dart:ffi';
import 'dart:io';

/// Loads the native anychat library dynamically based on the platform.
class NativeLibraryLoader {
  static DynamicLibrary? _lib;

  static DynamicLibrary get library {
    if (_lib != null) return _lib!;

    if (Platform.isAndroid) {
      _lib = DynamicLibrary.open('libanychat_android.so');
    } else if (Platform.isIOS) {
      // iOS loads from the app bundle (statically linked)
      _lib = DynamicLibrary.process();
    } else if (Platform.isMacOS) {
      // macOS: look for the dylib in the app bundle
      _lib = DynamicLibrary.open('libanychat_flutter_plugin.dylib');
    } else if (Platform.isLinux) {
      _lib = DynamicLibrary.open('libanychat_flutter_plugin.so');
    } else if (Platform.isWindows) {
      _lib = DynamicLibrary.open('anychat_flutter_plugin.dll');
    } else {
      throw UnsupportedError(
          'Platform ${Platform.operatingSystem} is not supported');
    }

    return _lib!;
  }
}
