import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

/// Dart FFI wrapper around the SWIG-generated C bindings.
/// Used internally by the Flutter package in packages/flutter/.
class AnyChatClientNative {
  static final DynamicLibrary _lib = _loadLib();

  static DynamicLibrary _loadLib() {
    if (Platform.isAndroid) {
      return DynamicLibrary.open('libanychat_android.so');
    } else if (Platform.isIOS) {
      return DynamicLibrary.process();
    }
    throw UnsupportedError('Unsupported platform: ${Platform.operatingSystem}');
  }

  // TODO: bind SWIG-generated C functions via _lib.lookup<>()
}
