#pragma once

#include <string>
#include <vector>

namespace fasthub::core {

/// @brief Permissions requested by plugins.
enum class PluginPermission {
    FilesystemRead,
    FilesystemWrite,
    Network,
    MessagesRead,
    MessagesSend,
    AI,
    Notifications
};

/// @brief Plugin manifest structure (JSON-based schema).
struct PluginManifest {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::vector<PluginPermission> permissions;
};

/// @brief Abstract interface for FastHub plugins.
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /// @brief Returns plugin manifest.
    virtual PluginManifest getManifest() const = 0;

    /// @brief Initializes plugin upon loading.
    virtual void initialize() = 0;

    /// @brief Shuts down plugin before unloading.
    virtual void shutdown() = 0;
};

} // namespace fasthub::core
