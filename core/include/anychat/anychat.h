/*
 * anychat.h — AnyChat SDK C interface
 *
 * This is the single include entry point. Include only this file.
 *
 * All strings are UTF-8. All callbacks fire on an internal SDK thread;
 * do not call SDK functions from within a callback unless documented as
 * reentrant-safe. All SDK-allocated strings and lists must be freed with
 * the corresponding anychat_free_* function.
 *
 * Memory ownership:
 *   - String *inputs*  : caller-owned; SDK copies internally.
 *   - String *outputs* : SDK-allocated; caller frees with anychat_free_string().
 *   - List outputs     : SDK-allocated; caller frees with the matching
 *                        anychat_free_*_list() function.
 *   - Sub-module handles returned by anychat_client_get_*() are owned by the
 *     client and must NOT be freed individually.
 */
#pragma once

#include "auth.h"
#include "client.h"
#include "conversation.h"
#include "errors.h"
#include "file.h"
#include "friend.h"
#include "group.h"
#include "message.h"
#include "call.h"
#include "types.h"
#include "user.h"
#include "version.h"
