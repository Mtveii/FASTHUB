#pragma once

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>

#include "core/i_chat_provider.h"
#include "steam_presence.h"

namespace fasthub::providers::steam {

/// @brief Real Steam presence provider over the official Steam Web API.
///
/// Behavior (design TODO Phase 1):
/// - polling only (Steam has no push/webhooks), default 45s, never faster;
/// - one shared QNetworkAccessManager (keep-alive) for all requests;
/// - SQLite cache is written only when the presence snapshot actually
///   changed (diff against in-memory previous snapshot);
/// - any API error (bad key / 429 / timeout) puts the provider into a
///   degraded state and NEVER affects other modules;
/// - without an API key the provider stays in honest mock mode.
class SteamProvider : public QObject, public core::IChatProvider {
    Q_OBJECT
public:
    explicit SteamProvider(QObject* parent = nullptr);
    ~SteamProvider() override;

    core::ProviderStatus getStatus() const override;
    std::vector<core::Conversation> getConversations() override;
    std::vector<core::Message> getMessages(const std::string& conversationId) override;
    bool sendMessage(const std::string& conversationId, const std::string& text) override;

    /// @brief true if an API key is stored (real mode available).
    bool hasCredentials() const;

    /// @brief Stored values, exposed for UI prefill (DPAPI-protected).
    QString apiKey() const;
    QString steamId() const;

    /// @brief Persists the Web API key + own steamid64 via ISecureStorage.
    void saveCredentials(const QString& apiKey, const QString& steamId64);

    /// @brief Starts the presence poll loop (no-op without credentials).
    void startPolling(int intervalMs = 45000);
    void stopPolling();

    /// @brief Last human-readable poll status for the UI ("live", "degraded: ...").
    QString lastPollStatus() const { return m_lastStatus; }

    /// @brief In-memory presence snapshot for fast UI reads.
    QMap<QString, SteamPresence> presenceSnapshot() const;

signals:
    /// @brief Emitted after each poll tick that changed something (or failed).
    void presenceUpdated(QString status);

public slots:
    void pollNow();

private:
    void fetchFriendsList();
    void fetchPlayerSummaries(const QStringList& steamIds);
    void handleReply(QNetworkReply* reply, std::function<void(const QJsonDocument&)> onOk);
    void applySnapshot(const QMap<QString, SteamPresence>& fresh);
    void enterDegraded(const QString& reason);

    QTimer m_pollTimer;
    QNetworkAccessManager m_http;
    QMap<QString, SteamPresence> m_snapshot;
    QString m_lastStatus = QStringLiteral("mock");
    core::ProviderStatus m_status = core::ProviderStatus::Unavailable;
    bool m_fetchInProgress = false;
};

} // namespace fasthub::providers::steam
