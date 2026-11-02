#pragma once

#include <QDialog>
#include <memory>

#include "providers/discord/discord_oauth.h"

class QLabel;
class QLineEdit;

namespace fasthub::ui {

/// @brief Official Discord login via browser OAuth2 (public API only).
class DiscordLoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit DiscordLoginDialog(std::shared_ptr<providers::discord::DiscordOauthService> service,
                                QWidget *parent = nullptr);

private slots:
    void onSaveCredentials();
    void onStartLogin();
    void onStateChanged(const QString &step, const QString &details);

private:
    void setupUi();

    std::shared_ptr<providers::discord::DiscordOauthService> m_service;
    QLabel *m_statusLabel;
    QLineEdit *m_clientIdEdit;
    QLineEdit *m_clientSecretEdit;
};

} // namespace fasthub::ui
