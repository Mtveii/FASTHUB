#include "discord_provider.h"
#include "core/id_utils.h"

namespace fasthub::providers::discord {

DiscordProvider::DiscordProvider() = default;

core::ProviderStatus DiscordProvider::getStatus() const { return m_status; }

std::vector<core::Conversation> DiscordProvider::getConversations() {
    return {{core::IdUtils::makeInternalId("discord", "chan_1"), "FastHub Server / #general",
             "Hello from official Discord API!", 1718000000, 0}};
}

std::vector<core::Message> DiscordProvider::getMessages(const std::string &conversationId) {
    return {{core::IdUtils::makeInternalId("discord", "msg_1"), conversationId, "Developer",
             "Hello from official Discord API!", 1718000000, false, false}};
}

bool DiscordProvider::sendMessage(const std::string &conversationId, const std::string &text) {
    return !text.empty();
}

std::vector<core::DiscordChannel> DiscordProvider::getChannels() {
    return {{core::IdUtils::makeInternalId("discord", "chan_1"), "FastHub Server", "general"}};
}

} // namespace fasthub::providers::discord
