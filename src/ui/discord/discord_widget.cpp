#include "discord_widget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "discord_login_dialog.h"

namespace fasthub::ui {

DiscordWidget::DiscordWidget(std::shared_ptr<providers::discord::DiscordProvider> discordProvider, QWidget* parent)
    : QWidget(parent), m_discordProvider(discordProvider),
      m_oauthService(std::make_shared<providers::discord::DiscordOauthService>(this)) {
    setupUi();
    loadChannels();
}

void DiscordWidget::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Discord Guilds & Channels (Official API)", this));

    m_channelsListWidget = new QListWidget(this);
    layout->addWidget(m_channelsListWidget);

    // Official OAuth2 browser login. A phone-scanned QR handshake is part of
    // Discord's private client protocol and is intentionally NOT used
    // (project rules: official API only).
    QPushButton* loginButton = new QPushButton("Login with Discord (browser OAuth2)", this);
    loginButton->setToolTip("Official OAuth2 flow; token stored via DPAPI");
    connect(loginButton, &QPushButton::clicked, this, &DiscordWidget::onLoginClicked);
    layout->addWidget(loginButton);
}

void DiscordWidget::loadChannels() {
    if (!m_discordProvider) return;

    auto channels = m_discordProvider->getChannels();
    m_channelsListWidget->clear();
    for (const auto& ch : channels) {
        QString itemText = QString("%1 / #%2").arg(QString::fromStdString(ch.guildName), QString::fromStdString(ch.channelName));
        m_channelsListWidget->addItem(itemText);
    }
}

void DiscordWidget::onLoginClicked() {
    DiscordLoginDialog dialog(m_oauthService, this);
    dialog.exec();
}

} // namespace fasthub::ui
