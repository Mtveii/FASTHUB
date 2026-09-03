#include <QApplication>
#include <QStandardPaths>
#include <QDir>

#include "infrastructure/logger.h"
#include "infrastructure/crash_handler.h"
#include "database/database_manager.h"
#include "plugins/plugin_manager.h"
#include "ui/main_window.h"
#include "ui/setup_dialog.h"
#include "ui/theme_manager.h"
#include "ui/icon_util.h"

#include <QApplication>
#include <QFont>

namespace {

constexpr const char* kSettingWorkspacePath = "workspace.path";
constexpr const char* kSettingOnboardingDone = "onboarding.done";

/// @brief Returns the saved notes directory; runs first-run onboarding if
///        the user has not chosen one yet.
QString resolveWorkspacePath() {
    auto& db = fasthub::database::DatabaseManager::instance();

    QString path = db.getSetting(kSettingWorkspacePath).value_or(QString());
    const bool onboardingDone = db.getSetting(kSettingOnboardingDone).value_or(QString()) == QStringLiteral("1");

    if (onboardingDone && !path.isEmpty()) {
        return path;
    }

    fasthub::ui::SetupDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        path = dialog.workspacePath();
        db.setSetting(kSettingWorkspacePath, path);
    }
    // Mark done even if skipped, so we don't nag every launch; the user can
    // still pick a folder later from the Workspace tab.
    db.setSetting(kSettingOnboardingDone, "1");
    return path;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("FastHub");
    app.setOrganizationName("FastHubTeam");

    // Brand identity: bundled fonts (Manrope/Inter/JetBrains Mono) + logo.
    fasthub::ui::IconUtil::loadBundledFonts();
    QFont appFont(QStringLiteral("Inter"), 10);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);
    app.setWindowIcon(fasthub::ui::IconUtil::logoIcon());

    // Initialize logger
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
    fasthub::infrastructure::Logger::instance().init(appDataDir + "/app.log");
    fasthub::infrastructure::CrashHandler::install(appDataDir);
    fasthub::infrastructure::Logger::instance().info("App", "FastHub starting up...");

    // Initialize database
    QString dbPath = appDataDir + "/fasthub.sqlite";
    if (!fasthub::database::DatabaseManager::instance().initialize(dbPath)) {
        fasthub::infrastructure::Logger::instance().error("App", "Failed to initialize database.");
    }

    // Apply saved theme (or default) before any window is shown.
    fasthub::ui::ThemeManager::instance().initialize();

    // First run: ask where the user's notes live (any drive).
    const QString workspacePath = resolveWorkspacePath();
    if (!workspacePath.isEmpty()) {
        fasthub::infrastructure::Logger::instance().info("App", "Workspace path: " + workspacePath);
    }

    // Show main window
    fasthub::ui::MainWindow mainWindow;
    mainWindow.setInitialWorkspacePath(workspacePath);
    mainWindow.show();

    int result = app.exec();

    fasthub::plugins::PluginManager::instance().shutdownAll();
    fasthub::infrastructure::Logger::instance().info("App", "FastHub shutting down.");
    return result;
}
