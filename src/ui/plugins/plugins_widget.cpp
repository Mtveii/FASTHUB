#include "plugins_widget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

#include "appearance_widget.h"

namespace fasthub::ui {

class SampleTranslatorPlugin : public core::IPlugin {
public:
    core::PluginManifest getManifest() const override {
        return {
            "translator",
            "Markdown Translator Plugin",
            "1.0.0",
            "Translates markdown text between languages.",
            {core::PluginPermission::FilesystemRead, core::PluginPermission::Network}
        };
    }
    void initialize() override {}
    void shutdown() override {}
};

PluginsWidget::PluginsWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    refreshList();
}

void PluginsWidget::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget(this);

    // Tab 1: Plugins
    QWidget* pluginsTab = new QWidget(this);
    QVBoxLayout* pluginsLayout = new QVBoxLayout(pluginsTab);
    pluginsLayout->addWidget(new QLabel("Installed Plugins & Permissions Sandbox", this));

    m_pluginsListWidget = new QListWidget(this);
    pluginsLayout->addWidget(m_pluginsListWidget);

    m_installButton = new QPushButton("Install Sample Plugin (Translator)", this);
    connect(m_installButton, &QPushButton::clicked, this, &PluginsWidget::onInstallPluginClicked);
    pluginsLayout->addWidget(m_installButton);
    m_tabs->addTab(pluginsTab, "Plugins");

    // Tab 2: Appearance (themes)
    m_tabs->addTab(new AppearanceWidget(this), "Appearance");

    layout->addWidget(m_tabs);
}

void PluginsWidget::refreshList() {
    m_pluginsListWidget->clear();
    auto loaded = plugins::PluginManager::instance().getLoadedPlugins();
    for (const auto& p : loaded) {
        QString itemText = QString("%1 v%2 — %3").arg(
            QString::fromStdString(p.name),
            QString::fromStdString(p.version),
            QString::fromStdString(p.description)
        );
        m_pluginsListWidget->addItem(itemText);
    }
}

void PluginsWidget::onInstallPluginClicked() {
    auto plugin = std::make_shared<SampleTranslatorPlugin>();
    auto manifest = plugin->getManifest();

    QString msg = QString("Plugin '%1' (v%2) requests the following permissions:\n\n"
                          "- Filesystem Read\n- Network Access\n\nDo you want to approve and install?")
                  .arg(QString::fromStdString(manifest.name))
                  .arg(QString::fromStdString(manifest.version));

    auto reply = QMessageBox::question(this, "Install Plugin & Permissions", msg, QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (plugins::PluginManager::instance().registerPlugin(plugin, true)) {
            QMessageBox::information(this, "Success", "Plugin installed successfully.");
            refreshList();
        } else {
            QMessageBox::warning(this, "Error", "Failed to install plugin.");
        }
    }
}

} // namespace fasthub::ui
