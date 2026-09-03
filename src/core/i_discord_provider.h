#pragma once

#include "core/i_chat_provider.h"
#include <string>
#include <vector>

namespace fasthub::core {

/// @brief Represents a Discord server guild and text channel.
struct DiscordChannel {
    std::string internalId;
    std::string guildName;
    std::string channelName;
};

/// @brief Abstract interface for Discord provider strictly adhering to official API.
class IDiscordProvider : public IChatProvider {
public:
    ~IDiscordProvider() override = default;

    /// @brief Retrieves list of accessible guilds and channels.
    virtual std::vector<DiscordChannel> getChannels() = 0;
};

} // namespace fasthub::core
