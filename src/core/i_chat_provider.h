#pragma once

#include <string>
#include <vector>
#include "core/models/conversation.h"
#include "core/models/message.h"

namespace fasthub::core {

/// @brief Status of a provider module.
enum class ProviderStatus {
    Available,
    Connecting,
    Unavailable,
    AuthenticationRequired
};

/// @brief Abstract interface for chat-based providers (Telegram, Discord, Steam).
class IChatProvider {
public:
    virtual ~IChatProvider() = default;

    /// @brief Returns current operational status of the provider.
    virtual ProviderStatus getStatus() const = 0;

    /// @brief Retrieves list of conversations.
    virtual std::vector<Conversation> getConversations() = 0;

    /// @brief Retrieves messages for a specific conversation.
    /// @param conversationId The unified conversation ID.
    virtual std::vector<Message> getMessages(const std::string& conversationId) = 0;

    /// @brief Sends a text message to a conversation.
    /// @param conversationId The unified conversation ID.
    /// @param text Message body.
    /// @return true if successfully sent/queued, false otherwise.
    virtual bool sendMessage(const std::string& conversationId, const std::string& text) = 0;
};

} // namespace fasthub::core
