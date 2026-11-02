#pragma once

#include <QObject>
#include <QTcpServer>

class QNetworkAccessManager;

namespace fasthub::providers::discord {

/// @brief Official browser-based OAuth2 login for Discord.
///
/// Flow (fully within Discord's public API):
/// 1. Opens the system browser on the /authorize URL of a registered app.
/// 2. Captures the redirect on a local loopback HTTP listener (127.0.0.1).
/// 3. Exchanges ?code= for an access token via the token endpoint.
/// 4. Stores tokens via ISecureStorage (DPAPI), never in plain files.
///
/// Requires client_id/client_secret of a Discord application
/// (discord.com/developers/applications) saved via saveAppCredentials().
class DiscordOauthService : public QObject {
    Q_OBJECT
public:
    explicit DiscordOauthService(QObject *parent = nullptr);

    static constexpr quint16 kCallbackPort = 8976;
    static constexpr const char *kRedirectUri = "http://127.0.0.1:8976/callback";
    static constexpr const char *kScope = "identify guilds";

    /// @brief true if client_id/client_secret are stored.
    bool hasCredentials() const;

    /// @brief Persists application credentials via ISecureStorage.
    void saveAppCredentials(const QString &clientId, const QString &clientSecret);

public slots:
    /// @brief Starts the browser + loopback-listener flow.
    void beginLogin();

private slots:
    void onCallbackConnection();

signals:
    /// @brief step: opened|waiting|code_captured|authorized|error
    void loginStateChanged(QString step, QString details);

private:
    void exchangeCodeForToken(const QString &code);
    void finishListener();

    QTcpServer m_listener;
    QNetworkAccessManager *m_http = nullptr;
};

} // namespace fasthub::providers::discord
