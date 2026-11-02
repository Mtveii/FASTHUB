#include "steam_provider.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>

#include "core/id_utils.h"
#include "core/models/conversation.h"
#include "database/database_manager.h"
#include "infrastructure/logger.h"
#include "infrastructure/secure_storage.h"

namespace fasthub::providers::steam {

namespace {
constexpr const char *kKeyApiKey = "steam.api_key";
constexpr const char *kKeySteamId = "steam.steamid64";

constexpr const char *kFriendListUrl = "https://api.steampowered.com/ISteamUser/GetFriendList/v1/";
constexpr const char *kSummariesUrl =
    "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/";

QString storedSecret(const char *key) {
    infrastructure::SecureStorage storage;
    return QString::fromStdString(storage.getSecret(key).value_or(std::string()));
}

core::Conversation toConversation(const SteamPresence &p) {
    core::Conversation c;
    c.internalId = core::IdUtils::makeInternalId("steam", p.steamId.toStdString());
    c.title = p.personaName.toStdString();
    c.lastMessageText = p.presenceLine().toStdString();
    c.lastMessageTimestamp = QDateTime::currentSecsSinceEpoch();
    c.unreadCount = 0;
    return c;
}
} // namespace

SteamProvider::SteamProvider(QObject *parent) : QObject(parent) {
    m_pollTimer.setSingleShot(false);
    connect(&m_pollTimer, &QTimer::timeout, this, &SteamProvider::pollNow);
}

SteamProvider::~SteamProvider() { stopPolling(); }

bool SteamProvider::hasCredentials() const { return !apiKey().isEmpty() && !steamId().isEmpty(); }

void SteamProvider::saveCredentials(const QString &apiKeyValue, const QString &steamId64) {
    infrastructure::SecureStorage storage;
    storage.setSecret(kKeyApiKey, apiKeyValue.toStdString());
    storage.setSecret(kKeySteamId, steamId64.toStdString());
    m_status = core::ProviderStatus::Unavailable; // re-evaluated on next poll
    infrastructure::Logger::instance().info("Steam",
                                            "Web API credentials saved (DPAPI-protected).");
}

QString SteamProvider::apiKey() const { return storedSecret(kKeyApiKey); }

QString SteamProvider::steamId() const { return storedSecret(kKeySteamId); }

core::ProviderStatus SteamProvider::getStatus() const { return m_status; }

std::vector<core::Conversation> SteamProvider::getConversations() {
    if (!hasCredentials()) {
        // Honest mock mode (no key configured yet).
        return {{core::IdUtils::makeInternalId("steam", "friend_1"), "Gaben",
                 "Hey, check out the new sale!", 1718000000, 1}};
    }
    return database::DatabaseManager::instance().getConversations();
}

std::vector<core::Message> SteamProvider::getMessages(const std::string &conversationId) {
    // Steam module is presence-only by design (see CONCEPT scope note).
    Q_UNUSED(conversationId);
    return {};
}

bool SteamProvider::sendMessage(const std::string &conversationId, const std::string &text) {
    // Steam friend-chat has no official third-party API - never pretend.
    Q_UNUSED(conversationId);
    Q_UNUSED(text);
    return false;
}

QMap<QString, SteamPresence> SteamProvider::presenceSnapshot() const { return m_snapshot; }

void SteamProvider::startPolling(int intervalMs) {
    if (!hasCredentials()) {
        infrastructure::Logger::instance().info("Steam",
                                                "Polling not started: no API credentials.");
        return;
    }
    // Rate-limit guard: never poll faster than 30s regardless of caller.
    const int safeInterval = qMax(intervalMs, 30000);
    m_pollTimer.start(safeInterval);
    infrastructure::Logger::instance().info(
        "Steam", QString("Presence polling started (%1ms).").arg(safeInterval));
    pollNow();
}

void SteamProvider::stopPolling() {
    if (m_pollTimer.isActive()) {
        m_pollTimer.stop();
    }
}

void SteamProvider::pollNow() {
    if (m_fetchInProgress) {
        return; // previous tick still running - skip, don't pile up
    }
    if (!hasCredentials()) {
        return;
    }
    m_fetchInProgress = true;
    fetchFriendsList();
}

void SteamProvider::fetchFriendsList() {
    QUrl url(QString::fromLatin1(kFriendListUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), apiKey());
    query.addQueryItem(QStringLiteral("steamid"), steamId());
    query.addQueryItem(QStringLiteral("relationship"), QStringLiteral("friend"));
    url.setQuery(query);

    handleReply(m_http.get(QNetworkRequest(url)), [this](const QJsonDocument &doc) {
        const QStringList ids = SteamPresenceCodec::parseFriendIds(doc);
        if (ids.isEmpty()) {
            applySnapshot({});
            m_status = core::ProviderStatus::Available;
            m_lastStatus = QStringLiteral("live (no friends on list)");
            emit presenceUpdated(m_lastStatus);
            m_fetchInProgress = false;
            return;
        }
        fetchPlayerSummaries(ids);
    });
}

void SteamProvider::fetchPlayerSummaries(const QStringList &steamIds) {
    // API accepts up to 100 steamids per call.
    QStringList batch = steamIds;
    if (batch.size() > 100) {
        batch = batch.mid(0, 100);
    }

    QUrl url(QString::fromLatin1(kSummariesUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), apiKey());
    query.addQueryItem(QStringLiteral("steamids"), batch.join(QStringLiteral(",")));
    url.setQuery(query);

    handleReply(m_http.get(QNetworkRequest(url)), [this](const QJsonDocument &doc) {
        QMap<QString, SteamPresence> fresh;
        const auto players = SteamPresenceCodec::parsePlayerSummaries(doc);
        for (const auto &p : players) {
            fresh.insert(p.steamId, p);
        }
        applySnapshot(fresh);
        m_status = core::ProviderStatus::Available;
        m_lastStatus = QStringLiteral("live");
        emit presenceUpdated(m_lastStatus);
        m_fetchInProgress = false;
    });
}

void SteamProvider::handleReply(QNetworkReply *reply,
                                std::function<void(const QJsonDocument &)> onOk) {
    connect(reply, &QNetworkReply::finished, this, [this, reply, onOk]() {
        reply->deleteLater();
        m_fetchInProgress = false;

        if (reply->error() != QNetworkReply::NoError) {
            enterDegraded(reply->errorString());
            return;
        }
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 429) {
            enterDegraded(QStringLiteral("rate limited (429) - back off"));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull()) {
            enterDegraded(QStringLiteral("malformed JSON response"));
            return;
        }
        try {
            onOk(doc);
        } catch (const std::exception &e) {
            enterDegraded(QString::fromUtf8(e.what()));
        } catch (...) {
            enterDegraded(QStringLiteral("unknown parse error"));
        }
    });
}

void SteamProvider::applySnapshot(const QMap<QString, SteamPresence> &fresh) {
    // Write to cache ONLY on real diff (design TODO Phase 1).
    if (!SteamPresenceCodec::snapshotDiffers(m_snapshot, fresh)) {
        return;
    }
    m_snapshot = fresh;

    std::vector<core::Conversation> convs;
    convs.reserve(fresh.size());
    for (auto it = fresh.constBegin(); it != fresh.constEnd(); ++it) {
        convs.push_back(toConversation(it.value()));
    }
    if (!database::DatabaseManager::instance().saveConversations(convs)) {
        infrastructure::Logger::instance().warn("Steam", "Failed to persist presence snapshot.");
    }
}

void SteamProvider::enterDegraded(const QString &reason) {
    // MODULAR PROVIDERS: degrade this module only, never touch others.
    m_status = core::ProviderStatus::Unavailable;
    m_lastStatus = QStringLiteral("degraded: %1").arg(reason);
    infrastructure::Logger::instance().warn("Steam", m_lastStatus);
    emit presenceUpdated(m_lastStatus);
}

} // namespace fasthub::providers::steam
