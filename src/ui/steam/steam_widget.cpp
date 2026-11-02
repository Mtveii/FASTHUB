#include "steam_widget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>

#include "steam_settings_dialog.h"
#include "icon_util.h"

namespace fasthub::ui {

SteamWidget::SteamWidget(std::shared_ptr<providers::steam::SteamProvider> steamProvider,
                         QWidget *parent)
    : QWidget(parent), m_steamProvider(steamProvider) {
    setupUi();
    loadFriends();

    // Start polling only when credentials exist (lazy by design: no network
    // until the user configured the module).
    if (m_steamProvider && m_steamProvider->hasCredentials()) {
        m_steamProvider->startPolling();
    }

    // Debounce: bursts of presence updates -> single UI repaint per 500ms.
    m_uiRefreshTimer.setSingleShot(true);
    m_uiRefreshTimer.setInterval(500);
    connect(&m_uiRefreshTimer, &QTimer::timeout, this, &SteamWidget::loadFriends);

    if (m_steamProvider) {
        connect(m_steamProvider.get(), &providers::steam::SteamProvider::presenceUpdated, this,
                &SteamWidget::onPresenceUpdated);
    }
}

void SteamWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    auto *header = new QLabel("Steam Friends & Status", this);
    header->setProperty("heading", true);
    layout->addWidget(header);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setProperty("role", "muted");
    layout->addWidget(m_statusLabel);

    m_friendsListWidget = new QListWidget(this);
    layout->addWidget(m_friendsListWidget);

    auto *row = new QHBoxLayout();
    auto *settingsButton = new QPushButton("Web API Setup...", this);
    connect(settingsButton, &QPushButton::clicked, this, &SteamWidget::onSettingsClicked);
    row->addWidget(settingsButton);
    row->addStretch();
    layout->addLayout(row);
}

QIcon SteamWidget::statusDot(const providers::steam::SteamPresence &p) const {
    QColor color = QColor("#8A93A6"); // offline - muted
    if (p.isInGame())
        color = QColor("#66C0F0"); // in-game - steam blue
    else if (p.isOnline())
        color = QColor("#7BD88F"); // online - green

    QPixmap pm(14, 14);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, 10, 10);
    return QIcon(pm);
}

void SteamWidget::loadFriends() {
    if (!m_steamProvider)
        return;

    m_friendsListWidget->clear();

    const bool realMode = m_steamProvider->hasCredentials();
    if (!realMode) {
        m_statusLabel->setText("Mock mode - configure the Web API key to see real friends.");
        // Honest demo rows until the module is configured.
        const std::vector<providers::steam::SteamPresence> demo = {
            {QStringLiteral("demo:1"), QStringLiteral("Gaben"), 1,
             QStringLiteral("Counter-Strike 2")},
            {QStringLiteral("demo:2"), QStringLiteral("ValveFan99"), 0, QString()},
        };
        for (const auto &p : demo) {
            auto *item = new QListWidgetItem(m_friendsListWidget);
            item->setText(QString("%1 — %2").arg(p.personaName, p.presenceLine()));
            item->setIcon(statusDot(p));
        }
        return;
    }

    m_statusLabel->setText(QString("Status: %1").arg(m_steamProvider->lastPollStatus()));

    const auto snapshot = m_steamProvider->presenceSnapshot();
    for (auto it = snapshot.constBegin(); it != snapshot.constEnd(); ++it) {
        const auto &p = it.value();
        auto *item = new QListWidgetItem(m_friendsListWidget);
        item->setText(QString("%1 — %2").arg(p.personaName, p.presenceLine()));
        item->setIcon(statusDot(p));
    }
}

void SteamWidget::onPresenceUpdated(const QString &status) {
    Q_UNUSED(status);
    m_uiRefreshTimer.start();
}

void SteamWidget::onSettingsClicked() {
    SteamSettingsDialog dialog(m_steamProvider, this);
    dialog.exec();
    loadFriends();
}

} // namespace fasthub::ui
