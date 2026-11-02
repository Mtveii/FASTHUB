#pragma once

#include <string>
#include <cstdint>

namespace fasthub::core {

/// @brief Unified conversation model across all chat providers.
struct Conversation {
    std::string internalId; // Format: provider:externalId
    std::string title;
    std::string lastMessageText;
    int64_t lastMessageTimestamp = 0;
    int unreadCount = 0;
};

} // namespace fasthub::core
