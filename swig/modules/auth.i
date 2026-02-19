%module anychat

// Callbacks: use directors so target language can override
%feature("director") anychat::AuthManager;

%include "anychat/auth.h"
