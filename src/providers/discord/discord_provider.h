#pragma once

#include "core/i_discord_provider.h"

namespace fasthub::providers::discord {

class DiscordProvider : public core::IDiscordProvider {
public:
    DiscordProvider();
    ~DiscordProvider() override = default;

    core::ProviderStatus getStatus() const override;
    std::vector<core::Conversation> getConversations() override;
    std::vector<core::Message> getMessages(const std::string &conversationId) override;
    bool sendMessage(const std::string &conversationId, const std::string &text) override;
    std::vector<core::DiscordChannel> getChannels() override;

private:
    core::ProviderStatus m_status = core::ProviderStatus::Available;
};

} // namespace fasthub::providers::discord
