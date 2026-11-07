#include "main_window.h"
#include "workspace_widget.h"
#include "search/search_dialog.h"
#include "ai/ai_widget.h"
#include "plugins/plugins_widget.h"
#include "developer/developer_dialog.h"
#include "web_embed_widget.h"
#include "telegram/ayugram_embed_widget.h"
#include "obsidian/obsidian_embed_widget.h"
#include "icon_util.h"
#include "interface_settings.h"
#include "context_broker.h"
#include "animation_util.h"
#include "style_tokens.h"

#include <QLabel>
#include <QShortcut>
#include <QKeySequence>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace fasthub::ui {

namespace {

struct TabEntry {
    const char *iconResource;
    const char *title;
    QColor accent;
};

// Design-spec accents: one brand color per integration.
const TabEntry kTabs[] = {
    {":/icons/rail-workspace.svg", "Workspace", QColor("#6E7BFF")},
    {":/icons/rail-telegram.svg", "Telegram", QColor("#2AABEE")},
    {":/icons/rail-discord.svg", "Discord", QColor("#5865F2")},
    {":/icons/rail-steam.svg", "Steam", QColor("#66C0F0")},
    {":/icons/rail-ai.svg", "AI", QColor("#B388FF")},
    {":/icons/rail-plugins.svg", "Plugins", QColor("#8A93A6")},
};

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Initialize services
    m_telegramProvider = std::make_shared<providers::telegram::TelegramProvider>();
    m_searchProvider = std::make_shared<search::SearchProvider>();
    m_aiProvider = std::make_shared<providers::ai::AIProvider>(core::AIProviderType::OpenAI);
    m_steamProvider = std::make_shared<providers::steam::SteamProvider>();
    m_discordProvider = std::make_shared<providers::discord::DiscordProvider>();
    m_syncService = std::make_unique<sync::SyncService>(m_telegramProvider, this);
    m_syncService->startSync(30000);

    setWindowIcon(IconUtil::logoIcon());

    // Editable interface metrics (persisted).
    InterfaceSettings::instance().initialize();
    QFont windowFont = font();
    windowFont.setPointSize(InterfaceSettings::instance().fontSize());
    setFont(windowFont);

    setupUi();

    // Shortcuts
    QShortcut *searchShortcut = new QShortcut(QKeySequence("Ctrl+K"), this);
    connect(searchShortcut, &QShortcut::activated, this, &MainWindow::openGlobalSearch);

    QShortcut *devShortcut = new QShortcut(QKeySequence("F12"), this);
    connect(devShortcut, &QShortcut::activated, this, &MainWindow::openDeveloperMode);

    // Ctrl+1..6: direct tab navigation (design TODO "Горячие клавиши").
    for (int i = 0; i < 6; ++i) {
        QShortcut *nav = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)), this);
        const int target = i;
        connect(nav, &QShortcut::activated, this, [this, target]() {
            if (target < m_topTabs->count()) {
                m_topTabs->setCurrentIndex(target);
            }
        });
    }
}

QIcon MainWindow::providerIcon(const QString &resource, const QColor &color) const {
    return IconUtil::tintedIcon(resource, color, InterfaceSettings::instance().tabIconSize());
}

void MainWindow::setupUi() {
    setWindowTitle("FastHub");
    resize(1200, 800);

    // Browser-style top tabs (owner request: "переключение страниц наверху
    // как в браузерах"). Icon + title per provider.
    m_topTabs = new QTabWidget(this);
    m_topTabs->setDocumentMode(true);
    m_topTabs->setTabPosition(QTabWidget::North);
    m_topTabs->setMovable(false);

    m_workspaceWidget = new WorkspaceWidget(this);
    m_topTabs->addTab(m_workspaceWidget, providerIcon(kTabs[0].iconResource, kTabs[0].accent),
                      kTabs[0].title);

    // Telegram: the full AyuGram desktop client embedded into the tab
    // (separate process + HWND capture = mere aggregation; GPL stays with
    // AyuGram). Official web client is the in-tab fallback.
    m_topTabs->addTab(new AyuGramEmbedWidget(this),
                      providerIcon(kTabs[1].iconResource, kTabs[1].accent), kTabs[1].title);

    // Discord: embedded official web app. Login once - session persists.
    m_topTabs->addTab(new WebEmbedWidget(QStringLiteral("discord"),
                                         QStringLiteral("https://discord.com/app"), this),
                      providerIcon(kTabs[2].iconResource, kTabs[2].accent), kTabs[2].title);

    // Obsidian: native desktop client embedded (separate process + HWND capture).
    m_topTabs->addTab(new ObsidianEmbedWidget(this),
                      providerIcon(kTabs[0].iconResource, kTabs[0].accent),
                      QStringLiteral("Obsidian"));

    // Steam: embedded web chat (friends + chat after one login).
    m_topTabs->addTab(new WebEmbedWidget(QStringLiteral("steam"),
                                         QStringLiteral("https://steamcommunity.com/chat"), this),
                      providerIcon(kTabs[3].iconResource, kTabs[3].accent), kTabs[3].title);

    // AI Assistant: pulls cross-module context on entry.
    m_aiWidget = new AIWidget(m_aiProvider, this);
    m_topTabs->addTab(m_aiWidget, providerIcon(kTabs[4].iconResource, kTabs[4].accent),
                      kTabs[4].title);

    PluginsWidget *pluginsWidget = new PluginsWidget(this);
    m_topTabs->addTab(pluginsWidget, providerIcon(kTabs[5].iconResource, kTabs[5].accent),
                      kTabs[5].title);

    connect(m_topTabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 4 && m_aiWidget) {
            m_aiWidget->pullContext();
        }
    });

    connect(
        &InterfaceSettings::instance(), &InterfaceSettings::fontSizeChanged, this,
        [this](int pt) {
            QFont f = font();
            f.setPointSize(pt);
            setFont(f);
        },
        Qt::UniqueConnection);

    connect(
        &InterfaceSettings::instance(), &InterfaceSettings::tabIconSizeChanged, this,
        [this](int px) {
            for (int i = 0; i < m_topTabs->count(); ++i) {
                m_topTabs->setTabIcon(
                    i, IconUtil::tintedIcon(kTabs[i].iconResource, kTabs[i].accent, px));
            }
        },
        Qt::UniqueConnection);

    setCentralWidget(m_topTabs);
}

void MainWindow::setInitialWorkspacePath(const QString &path) {
    if (m_workspaceWidget && !path.isEmpty()) {
        m_workspaceWidget->openWorkspacePath(path);
    }
}

void MainWindow::openGlobalSearch() {
    SearchDialog dialog(m_searchProvider, this);
    dialog.exec();
}

void MainWindow::openDeveloperMode() {
    DeveloperDialog dialog(this);
    dialog.exec();
}

} // namespace fasthub::ui
