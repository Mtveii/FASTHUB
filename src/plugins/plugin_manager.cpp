#include "plugin_manager.h"
#include "infrastructure/logger.h"

namespace fasthub::plugins {

PluginManager& PluginManager::instance() {
    static PluginManager s_instance;
    return s_instance;
}

bool PluginManager::registerPlugin(std::shared_ptr<core::IPlugin> plugin, bool approvedByUser) {
    if (!plugin || !approvedByUser) {
        infrastructure::Logger::instance().warn("Plugins", "Plugin registration rejected or not approved by user.");
        return false;
    }

    // A misbehaving plugin must never take the whole application down
    // (AGENTS.md §2.3): isolate its lifecycle calls.
    try {
        plugin->initialize();
    } catch (const std::exception& e) {
        infrastructure::Logger::instance().error("Plugins", QString("Plugin '%1' threw during initialize(): %2")
            .arg(QString::fromStdString(plugin->getManifest().name), QString::fromUtf8(e.what())));
        return false;
    } catch (...) {
        infrastructure::Logger::instance().error("Plugins", QString("Plugin '%1' threw unknown exception during initialize().")
            .arg(QString::fromStdString(plugin->getManifest().name)));
        return false;
    }

    m_plugins.push_back(plugin);
    infrastructure::Logger::instance().info("Plugins", "Plugin registered and initialized successfully: " + QString::fromStdString(plugin->getManifest().name));
    return true;
}

void PluginManager::shutdownAll() {
    for (auto& plugin : m_plugins) {
        if (!plugin) continue;
        try {
            plugin->shutdown();
        } catch (const std::exception& e) {
            infrastructure::Logger::instance().error("Plugins", QString("Plugin '%1' threw during shutdown(): %2")
                .arg(QString::fromStdString(plugin->getManifest().name), QString::fromUtf8(e.what())));
        } catch (...) {
            infrastructure::Logger::instance().error("Plugins", QString("Plugin '%1' threw unknown exception during shutdown().")
                .arg(QString::fromStdString(plugin->getManifest().name)));
        }
    }
    m_plugins.clear();
}

std::vector<core::PluginManifest> PluginManager::getLoadedPlugins() const {
    std::vector<core::PluginManifest> manifests;
    for (const auto& p : m_plugins) {
        manifests.push_back(p->getManifest());
    }
    return manifests;
}

} // namespace fasthub::plugins
