#pragma once

#include <QWidget>
#include <QListWidget>
#include <memory>
#include "providers/discord/discord_provider.h"
#include "providers/discord/discord_oauth.h"

namespace fasthub::ui {

class DiscordWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiscordWidget(std::shared_ptr<providers::discord::DiscordProvider> discordProvider,
                           QWidget *parent = nullptr);
    ~DiscordWidget() override = default;

private:
    void setupUi();
    void loadChannels();
    void onLoginClicked();

    std::shared_ptr<providers::discord::DiscordProvider> m_discordProvider;
    std::shared_ptr<providers::discord::DiscordOauthService> m_oauthService;
    QListWidget *m_channelsListWidget;
};

} // namespace fasthub::ui
