%module anychat

%{
#include "anychat/types.h"
#include "anychat/auth.h"
#include "anychat/message.h"
#include "anychat/client.h"
%}

// Enable STL types
%include <std_string.i>
%include <std_vector.i>
%include <std_shared_ptr.i>
%include <stdint.i>

// Shared pointer support
%shared_ptr(anychat::AnyChatClient)

// Include sub-modules
%include "modules/types.i"
%include "modules/auth.i"
%include "modules/message.i"
%include "modules/client.i"
