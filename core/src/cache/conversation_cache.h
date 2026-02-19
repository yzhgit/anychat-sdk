#pragma once

#include "anychat/types.h"

#include <algorithm>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace anychat::cache {

// Maintains an in-memory sorted list of Conversation objects.
//
// Sort order (stable):
//   1. Pinned conversations first, ordered by pin_time descending.
//   2. Non-pinned conversations, ordered by last_msg_time descending.
//
// All public methods are thread-safe.
class ConversationCache {
public:
    // Replace the entire list (called on full sync).
    void setAll(std::vector<Conversation> convs) {
        std::lock_guard<std::mutex> lk(mutex_);
        convs_ = std::move(convs);
        sort();
    }

    // Insert (if conv_id is new) or update (if conv_id already exists),
    // then re-sort.
    void upsert(Conversation conv) {
        std::lock_guard<std::mutex> lk(mutex_);
        for (auto& c : convs_) {
            if (c.conv_id == conv.conv_id) {
                c = std::move(conv);
                sort();
                return;
            }
        }
        convs_.push_back(std::move(conv));
        sort();
    }

    // Remove the conversation with the given conv_id (no-op if not found).
    void remove(const std::string& conv_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        convs_.erase(
            std::remove_if(convs_.begin(), convs_.end(),
                           [&](const Conversation& c) {
                               return c.conv_id == conv_id;
                           }),
            convs_.end());
        // No sort needed after removal.
    }

    // Return a sorted snapshot (copy).
    std::vector<Conversation> getAll() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return convs_;
    }

    // Return a single conversation by conv_id.
    std::optional<Conversation> get(const std::string& conv_id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        for (const auto& c : convs_) {
            if (c.conv_id == conv_id) return c;
        }
        return std::nullopt;
    }

    // Increment unread_count for the given conversation.
    // Has no effect if the conversation is muted.
    void incrementUnread(const std::string& conv_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        for (auto& c : convs_) {
            if (c.conv_id == conv_id) {
                if (!c.is_muted) ++c.unread_count;
                return;
            }
        }
    }

    // Reset unread_count to 0 for the given conversation.
    void clearUnread(const std::string& conv_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        for (auto& c : convs_) {
            if (c.conv_id == conv_id) {
                c.unread_count = 0;
                return;
            }
        }
    }

    // Update last-message metadata and re-sort (since last_msg_time affects
    // sort order for non-pinned conversations).
    void setLastMessage(const std::string& conv_id,
                        const std::string& msg_id,
                        const std::string& text,
                        int64_t            timestamp_ms) {
        std::lock_guard<std::mutex> lk(mutex_);
        for (auto& c : convs_) {
            if (c.conv_id == conv_id) {
                c.last_msg_id   = msg_id;
                c.last_msg_text = text;
                c.last_msg_time_ms = timestamp_ms;
                sort();
                return;
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        convs_.clear();
    }

private:
    // Sort convs_ in place.  Must be called with mutex_ held.
    void sort() {
        std::stable_sort(convs_.begin(), convs_.end(),
            [](const Conversation& a, const Conversation& b) {
                // Pinned conversations come first.
                if (a.is_pinned != b.is_pinned)
                    return a.is_pinned > b.is_pinned;

                // Among pinned: higher pin_time_ms first.
                if (a.is_pinned && b.is_pinned)
                    return a.pin_time_ms > b.pin_time_ms;

                // Among non-pinned: most-recently-active first.
                return a.last_msg_time_ms > b.last_msg_time_ms;
            });
    }

    mutable std::mutex        mutex_;
    std::vector<Conversation> convs_;
};

} // namespace anychat::cache
