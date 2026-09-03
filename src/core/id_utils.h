#pragma once

#include <string>

namespace fasthub::core {

/// @brief Helper utilities for unified entity ID generation and parsing.
class IdUtils {
public:
    /// @brief Generates a unified internal ID in format `provider:externalId`.
    static std::string makeInternalId(const std::string& providerName, const std::string& externalId) {
        return providerName + ":" + externalId;
    }
};

} // namespace fasthub::core
