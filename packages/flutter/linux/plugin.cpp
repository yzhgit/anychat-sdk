// Thin wrapper for Flutter FFI plugin
// This file ensures the shared library exports all symbols from anychat_c

// Include all C API headers to ensure symbols are exported
#include <anychat_c/anychat_c.h>

// No additional code needed - the linker will export all symbols from anychat_c
// through the PUBLIC link dependency specified in CMakeLists.txt
