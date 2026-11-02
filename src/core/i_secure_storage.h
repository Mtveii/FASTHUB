#pragma once

#include <string>
#include <optional>

namespace fasthub::core {

/// @brief Interface for secure storage of tokens, passwords, and sensitive keys.
class ISecureStorage {
public:
    virtual ~ISecureStorage() = default;

    /// @brief Stores a sensitive string value securely associated with a key.
    /// @param key The identifier key.
    /// @param value The secret value to store.
    /// @return true if successfully stored, false otherwise.
    virtual bool setSecret(const std::string &key, const std::string &value) = 0;

    /// @brief Retrieves a secret value associated with a key.
    /// @param key The identifier key.
    /// @return std::optional containing the secret if found, std::nullopt otherwise.
    virtual std::optional<std::string> getSecret(const std::string &key) = 0;

    /// @brief Removes a secret from secure storage.
    /// @param key The identifier key.
    /// @return true if successfully removed or not found, false on error.
    virtual bool removeSecret(const std::string &key) = 0;
};

} // namespace fasthub::core
