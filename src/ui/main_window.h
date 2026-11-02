#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <memory>
#include "providers/telegram/telegram_provider.h"
#include "providers/ai/ai_provider.h"
#include "providers/steam/steam_provider.h"
#include "providers/discord/discord_provider.h"
#include "sync/sync_service.h"
#include "search/search_provider.h"

namespace fasthub::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    /// @brief Points the Workspace tab at @p path (from onboarding/settings).
    void setInitialWorkspacePath(const QString &path);

private slots:
    void openGlobalSearch();
    void openDeveloperMode();

private:
    void setupUi();
    QIcon providerIcon(const QString &resource, const QColor &color) const;

    QTabWidget *m_topTabs = nullptr;
    class WorkspaceWidget *m_workspaceWidget = nullptr;
    class AIWidget *m_aiWidget = nullptr;

    std::shared_ptr<providers::telegram::TelegramProvider> m_telegramProvider;
    std::shared_ptr<providers::ai::AIProvider> m_aiProvider;
    std::shared_ptr<providers::steam::SteamProvider> m_steamProvider;
    std::shared_ptr<providers::discord::DiscordProvider> m_discordProvider;
    std::shared_ptr<search::SearchProvider> m_searchProvider;
    std::unique_ptr<sync::SyncService> m_syncService;
};

} // namespace fasthub::ui
