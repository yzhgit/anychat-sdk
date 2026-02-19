%module anychat

%include "anychat/types.h"

%template(MessageList) std::vector<anychat::Message>;
