#pragma once

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QVector>

namespace fasthub::providers::steam {

/// @brief One friend's presence snapshot (Steam Web API terminology).
struct SteamPresence {
    QString steamId;        ///< 64-bit SteamID as string.
    QString personaName;    ///< Display name.
    int personaState = 0;   ///< 0 offline, 1 online, 2 busy, 3 away, 4 snooze...
    QString gameName;       ///< Non-empty when the friend is in-game.

    bool isOnline() const { return personaState != 0; }
    bool isInGame() const { return !gameName.isEmpty(); }

    /// @brief Unified conversation title line shown in the list.
    QString presenceLine() const {
        if (isInGame()) return QStringLiteral("Playing: %1").arg(gameName);
        switch (personaState) {
            case 1: return QStringLiteral("Online");
            case 2: return QStringLiteral("Busy");
            case 3: return QStringLiteral("Away");
            case 4: return QStringLiteral("Snooze");
            default: return QStringLiteral("Offline");
        }
    }
};

/// @brief Pure parsing/diff helpers for ISteamUser endpoints (no network).
namespace SteamPresenceCodec {

/// @brief Parses GetPlayerSummaries/v2 response into snapshots.
///        Malformed entries are skipped; never throws.
QVector<SteamPresence> parsePlayerSummaries(const QJsonDocument& doc);

/// @brief Parses GetFriendList/v1 response into a list of steamids.
QStringList parseFriendIds(const QJsonDocument& doc);

/// @brief true when the new snapshot differs from the cached one.
///        Used to avoid rewriting the SQLite cache on every poll tick.
bool snapshotDiffers(const QMap<QString, SteamPresence>& oldSnapshot,
                     const QMap<QString, SteamPresence>& newSnapshot);

} // namespace SteamPresenceCodec

} // namespace fasthub::providers::steam
