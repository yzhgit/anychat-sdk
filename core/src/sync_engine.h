#pragma once

#include "cache/conversation_cache.h"
#include "cache/message_cache.h"
#include "db/database.h"
#include "network/http_client.h"

#include <memory>
#include <string>

// Forward-declare nlohmann::json to avoid heavy include in header.
namespace nlohmann { class json; }

namespace anychat {

// SyncEngine performs incremental data sync against the POST /sync endpoint.
//
// Called by ConnectionManager (via the on_ready hook) each time the WebSocket
// connection is established.  It reads the persisted last_sync_time from the
// local database, collects the current per-conversation sequence numbers,
// posts the sync request, and merges the response into both the database and
// the in-memory caches.
class SyncEngine {
public:
    SyncEngine(db::Database*                         db,
               cache::ConversationCache*             conv_cache,
               cache::MessageCache*                  msg_cache,
               std::shared_ptr<network::HttpClient>  http);

    // Trigger an incremental sync.  Reads last_sync_time from the database,
    // builds the sync request, and posts it to /sync.  The response is merged
    // into the database and caches asynchronously on the HTTP worker thread.
    void sync();

private:
    // Parse and dispatch the raw JSON response body.
    void handleSyncResponse(const std::string& body);

    // Merge friend delta into the database.
    void mergeFriends(const nlohmann::json& friends_arr);

    // Merge group delta into the database.
    void mergeGroups(const nlohmann::json& groups_arr);

    // Upsert each session entry into the conversations table and conv_cache_.
    void mergeSessions(const nlohmann::json& sessions_arr);

    // Insert offline messages for each conversation into the messages table
    // and msg_cache_.
    void mergeConvMessages(const nlohmann::json& conversations_arr);

    db::Database*                           db_;
    cache::ConversationCache*               conv_cache_;
    cache::MessageCache*                    msg_cache_;
    std::shared_ptr<network::HttpClient>    http_;
};

} // namespace anychat
