#pragma once

#include "core/i_chat_provider.h"
#include <string>
#include <vector>

namespace fasthub::core {

/// @brief Represents a Steam friend profile and online status.
struct SteamFriend {
    std::string internalId;
    std::string personaName;
    bool isOnline = false;
    std::string currentGame;
};

/// @brief Abstract interface for Steam provider extending IChatProvider.
class ISteamProvider : public IChatProvider {
public:
    ~ISteamProvider() override = default;

    /// @brief Retrieves list of Steam friends and their statuses.
    virtual std::vector<SteamFriend> getFriends() = 0;
};

} // namespace fasthub::core
