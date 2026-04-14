// Thin wrapper for Flutter FFI plugin.
// This file ensures the shared library exports all symbols from the public AnyChat C API.

// Include the single public C API entry point.
#include <anychat/anychat.h>

// No additional code needed - the linker will export all public AnyChat symbols
// through the PUBLIC link dependency specified in CMakeLists.txt
