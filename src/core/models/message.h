#pragma once

#include <string>
#include <cstdint>

namespace fasthub::core {

/// @brief Unified message model across all chat providers.
struct Message {
    std::string internalId;     // Format: provider:externalId
    std::string conversationId; // Format: provider:externalConversationId
    std::string senderName;
    std::string text;
    int64_t timestamp = 0;
    bool isOutgoing = false;
    bool isPending = false;
};

} // namespace fasthub::core
