#pragma once

#include "core/i_plugin.h"
#include <memory>
#include <vector>

namespace fasthub::plugins {

class PluginManager {
public:
    static PluginManager& instance();

    bool registerPlugin(std::shared_ptr<core::IPlugin> plugin, bool approvedByUser);
    std::vector<core::PluginManifest> getLoadedPlugins() const;

    /// @brief Safely shuts down all registered plugins (exception-isolated).
    void shutdownAll();

private:
    PluginManager() = default;
    ~PluginManager() = default;

    std::vector<std::shared_ptr<core::IPlugin>> m_plugins;
};

} // namespace fasthub::plugins
